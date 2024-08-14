"""Validate the YAML files for GitHub Actions workflows.

TODO: b/359303016 - convert to use unittest
"""

import os
import re

import yaml

# Ensure every job is in the list of blocking jobs.
with open(
    os.path.join(os.path.dirname(__file__), '../workflows/test_runner.yml'), 'r'
) as f:
  data = yaml.safe_load(f)

  # List of all YAML files that are used by jobs in the test_runner.yml file.
  yaml_files = []

  # Get a list of all jobs in the test_runner, except for the blocking job and
  # the tag removal job, which is not always run.
  all_jobs = list(data['jobs'].keys())
  all_jobs.remove('all-blocking-tests')
  all_jobs.remove('remove-tag')

  passed = True
  blocking_jobs = data['jobs']['all-blocking-tests']['needs']

  for job in all_jobs:
    if 'uses' in data['jobs'][job]:
      yaml_files.append(
          os.path.join(
              os.path.dirname(__file__),
              '../workflows',
              os.path.basename(data['jobs'][job]['uses']),
          )
      )
    if job not in blocking_jobs:
      passed = False
      raise ValueError('Job %s is not in the list of blocking jobs.' % job)

  print('PASSED: All jobs are in the list of blocking jobs.')

# Ensure every job with a continuous prefix conditions every step on whether we
# are in a continuous run.
for file in yaml_files:
  with open(file, 'r') as f:
    data = yaml.safe_load(f)
    jobs = data['jobs']
    for job in jobs:
      if 'steps' not in jobs[job]:
        continue
      continuous_condition = 'inputs.continuous-prefix' in jobs[job]['name']
      steps = jobs[job]['steps']
      for step in steps:
        if continuous_condition and 'continuous-run' not in step.get('if', ''):
          raise ValueError(
              'Step %s in job %s does not check the continuous-run condition'
              % (step['name'], job)
          )
        if not continuous_condition and 'continuous-run' in step.get('if', ''):
          raise ValueError(
              'Step %s in job %s checks the continuous-run condition but '
              'the job does not contain the continuous-prefix'
              % (step['name'], job)
          )
print('PASSED: All steps in all jobs check the continuous-run condition.')

# Check to make sure the list of included branches matches the list of excluded
# branches in staleness_check.yml.
with open(
    os.path.join(os.path.dirname(__file__), '../workflows/staleness_check.yml'),
    'r',
) as f:
  regex_pattern = r"'(\d+\.x)'"
  data = yaml.safe_load(f)
  matrix = data['jobs']['test']['strategy']['matrix']
  included_branches = matrix['branch']
  # Main should be included in all test runs
  included_branches.remove('main')
  excludes = matrix['exclude']
  for entry in excludes:
    match = re.search(regex_pattern, entry['branch'])
    branch = match.group(1)
    if branch not in included_branches:
      raise ValueError(
          'Branch %s is excluded for presubmit runs but is not in the list of'
          ' matrix branches in staleness_check.yml.' % branch
      )
    included_branches.remove(branch)
  if included_branches:
    raise ValueError(
        'Branches %s are in the list of matrix branches but do not get excluded'
        ' for presubmit runs in staleness_check.yml.' % included_branches
    )
  print(
      'PASSED: The list of included branches matches the list of excluded'
      ' branches in staleness_check.yml.'
  )
