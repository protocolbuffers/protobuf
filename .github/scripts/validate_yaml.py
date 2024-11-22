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
        if 'name' in step:
          name = step['name']
        elif 'with' in step and 'name' in step['with']:
          name = step['with']['name']
        else:
          raise ValueError(
              'Step in job %s from file %s does not have a name.' % (job, file)
          )
        if continuous_condition and 'continuous-run' not in step.get('if', ''):
          raise ValueError(
              'Step %s in job %s from file %s does not check the continuous-run'
              ' condition' % (name, job, file)
          )
        if not continuous_condition and 'continuous-run' in step.get('if', ''):
          raise ValueError(
              'Step %s in job %s from file %s checks the continuous-run'
              ' condition but the job does not contain the continuous-prefix'
              % (name, job, file)
          )
print('PASSED: All steps in all jobs check the continuous-run condition.')
