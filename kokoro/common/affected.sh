set -ex

AFFECTED=$(git log --name-only --pretty=format: main..HEAD)

echo $AFFECTED

exit 1
