#!/bin/bash

template_package_xml=$1
release_version=$2
release_files=$3
out=$4

date=$(date +%Y-%m-%d)
time=$(date +%H:%M:%S)

php_version_array=(${release_version//RC/ })
api_version=${php_version_array[0]}
if [ ${#php_version_array[@]} -eq 2 ]; then
    rc=${php_version_array[1]}
    stability='beta'
else
    stability='stable'
fi

files="\\n"
for file in ${release_files//,/ }; do
  name=$(echo $file | grep .*[^/]\$ | sed -e "s;protobuf-${release_version}/;;")
  if [ ! -z $name ]; then
    if [[ $name =~ LICENSE$ ]]; then
      role='doc'
    else
      role='src'
    fi
    files+="    <file baseinstalldir=\"/\" name=\"${name}\" role=\"${role}\"/>\\n"
  fi
done

cat $template_package_xml |
sed -e "s;TEMPLATE_DATE;${date};" |
sed -e "s;TEMPLATE_TIME;${time};" |
sed -e "s;TEMPLATE_PHP_RELEASE;${release_version};" |
sed -e "s;TEMPLATE_PHP_API;${api_version};" |
sed -e "s;TEMPLATE_PHP_STABILITY;${stability};g" |
sed -e "s;TEMPLATE_FILES;${files};" > $out