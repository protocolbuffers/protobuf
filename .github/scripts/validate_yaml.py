"""Validate the YAML files for GitHub Actions workflows."""

import os

import yaml

# Ensure every job is in the list of blocking jobs.
with open(
    os.path.join(os.path.dirname(__file__), '../workflows/test_runner.yml'), 'r'
) as f:
  data = yaml.safe_load(f)

  all_jobs = data['jobs'].keys()
  blocking_jobs = data['jobs']['all_blocking_tests']['needs']
  for job in all_jobs:
    if job != 'all_blocking_tests':
      if job not in blocking_jobs:
        raise ValueError(
            'Job %s is not in the list of blocking jobs.'
            % (job)
        )

# Ensure every job with a continuous prefix conditions every step on whether we
# are in a continuous run.
for root, dirs, files in os.walk(
    os.path.join(os.path.dirname(__file__), '../workflows')
):
  for file in files:
    if file.endswith('.yml'):
      with open(os.path.join(root, file), 'r') as f:
        data = yaml.safe_load(f)
        if 'jobs' not in data:
          continue
        jobs = data['jobs']
        for job in jobs:
          if 'steps' not in jobs[job]:
            continue
          continuous_condition = 'inputs.continuous-prefix' in jobs[job]['name']
          steps = jobs[job]['steps']
          for step in steps:
            if 'if' in step:
              if continuous_condition:
                if 'continuous-run' not in step['if']:
                  raise ValueError(
                      'Step %s in job %s does not check the continuous-run '
                      'condition' % (step['name'], job)
                  )
              elif 'continuous-run' in step['if']:
                raise ValueError(
                    'Step %s in job %s checks the continuous-run condition but '
                    'the job does not contain the continuous-prefix'
                    % (step['name'], job)
                )
            elif continuous_condition:
              raise ValueError(
                  'Step %s in job %s does not check the continuous-run '
                  'condition' % (step['name'], job)
              )
