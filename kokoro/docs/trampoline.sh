#!/bin/bash
# Copied from https://github.com/googleapis/google-cloud-python/blob/main/.kokoro/trampoline.sh

set -eo pipefail

python3 "${KOKORO_GFILE_DIR}/trampoline_v1.py"  || ret_code=$?

chmod +x ${KOKORO_GFILE_DIR}/trampoline_cleanup.sh
${KOKORO_GFILE_DIR}/trampoline_cleanup.sh || true

exit ${ret_code}
