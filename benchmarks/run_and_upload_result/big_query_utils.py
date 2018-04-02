#!/usr/bin/env python2.7

import argparse
import json
import uuid
import httplib2

from apiclient import discovery
from apiclient.errors import HttpError
from oauth2client.client import GoogleCredentials

# 30 days in milliseconds
_EXPIRATION_MS = 30 * 24 * 60 * 60 * 1000
NUM_RETRIES = 3


def create_big_query():
    """Authenticates with cloud platform and gets a BiqQuery service object
  """
    creds = GoogleCredentials.get_application_default()
    return discovery.build(
        'bigquery', 'v2', credentials=creds, cache_discovery=False)


def create_dataset(biq_query, project_id, dataset_id):
    is_success = True
    body = {
        'datasetReference': {
            'projectId': project_id,
            'datasetId': dataset_id
        }
    }

    try:
        dataset_req = biq_query.datasets().insert(
            projectId=project_id, body=body)
        dataset_req.execute(num_retries=NUM_RETRIES)
    except HttpError as http_error:
        if http_error.resp.status == 409:
            print 'Warning: The dataset %s already exists' % dataset_id
        else:
            # Note: For more debugging info, print "http_error.content"
            print 'Error in creating dataset: %s. Err: %s' % (dataset_id,
                                                              http_error)
            is_success = False
    return is_success


def create_table(big_query, project_id, dataset_id, table_id, table_schema,
                 description):
    fields = [{
        'name': field_name,
        'type': field_type,
        'description': field_description
    } for (field_name, field_type, field_description) in table_schema]
    return create_table2(big_query, project_id, dataset_id, table_id, fields,
                         description)


def create_partitioned_table(big_query,
                             project_id,
                             dataset_id,
                             table_id,
                             table_schema,
                             description,
                             partition_type='DAY',
                             expiration_ms=_EXPIRATION_MS):
    """Creates a partitioned table. By default, a date-paritioned table is created with
  each partition lasting 30 days after it was last modified.
  """
    fields = [{
        'name': field_name,
        'type': field_type,
        'description': field_description
    } for (field_name, field_type, field_description) in table_schema]
    return create_table2(big_query, project_id, dataset_id, table_id, fields,
                         description, partition_type, expiration_ms)


def create_table2(big_query,
                  project_id,
                  dataset_id,
                  table_id,
                  fields_schema,
                  description,
                  partition_type=None,
                  expiration_ms=None):
    is_success = True

    body = {
        'description': description,
        'schema': {
            'fields': fields_schema
        },
        'tableReference': {
            'datasetId': dataset_id,
            'projectId': project_id,
            'tableId': table_id
        }
    }

    if partition_type and expiration_ms:
        body["timePartitioning"] = {
            "type": partition_type,
            "expirationMs": expiration_ms
        }

    try:
        table_req = big_query.tables().insert(
            projectId=project_id, datasetId=dataset_id, body=body)
        res = table_req.execute(num_retries=NUM_RETRIES)
        print 'Successfully created %s "%s"' % (res['kind'], res['id'])
    except HttpError as http_error:
        if http_error.resp.status == 409:
            print 'Warning: Table %s already exists' % table_id
        else:
            print 'Error in creating table: %s. Err: %s' % (table_id,
                                                            http_error)
            is_success = False
    return is_success


def patch_table(big_query, project_id, dataset_id, table_id, fields_schema):
    is_success = True

    body = {
        'schema': {
            'fields': fields_schema
        },
        'tableReference': {
            'datasetId': dataset_id,
            'projectId': project_id,
            'tableId': table_id
        }
    }

    try:
        table_req = big_query.tables().patch(
            projectId=project_id,
            datasetId=dataset_id,
            tableId=table_id,
            body=body)
        res = table_req.execute(num_retries=NUM_RETRIES)
        print 'Successfully patched %s "%s"' % (res['kind'], res['id'])
    except HttpError as http_error:
        print 'Error in creating table: %s. Err: %s' % (table_id, http_error)
        is_success = False
    return is_success


def insert_rows(big_query, project_id, dataset_id, table_id, rows_list):
    is_success = True
    body = {'rows': rows_list}
    try:
        insert_req = big_query.tabledata().insertAll(
            projectId=project_id,
            datasetId=dataset_id,
            tableId=table_id,
            body=body)
        res = insert_req.execute(num_retries=NUM_RETRIES)
        if res.get('insertErrors', None):
            print 'Error inserting rows! Response: %s' % res
            is_success = False
    except HttpError as http_error:
        print 'Error inserting rows to the table %s' % table_id
        is_success = False

    return is_success


def sync_query_job(big_query, project_id, query, timeout=5000):
    query_data = {'query': query, 'timeoutMs': timeout}
    query_job = None
    try:
        query_job = big_query.jobs().query(
            projectId=project_id,
            body=query_data).execute(num_retries=NUM_RETRIES)
    except HttpError as http_error:
        print 'Query execute job failed with error: %s' % http_error
        print http_error.content
    return query_job


    # List of (column name, column type, description) tuples
def make_row(unique_row_id, row_values_dict):
    """row_values_dict is a dictionary of column name and column value.
  """
    return {'insertId': unique_row_id, 'json': row_values_dict}
