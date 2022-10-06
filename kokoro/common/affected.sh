set -ex

echo $KOKORO_JOB_NAME

AFFECTED=$(git log --name-only --pretty=format: main..HEAD)

echo $AFFECTED

exit 1
