set -ex

git --no-pager log

AFFECTED=$(git log --name-only --pretty=format: main..HEAD)

echo $AFFECTED

exit 1
