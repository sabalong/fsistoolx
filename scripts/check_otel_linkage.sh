#!/bin/sh
set -eu

if ! command -v ldd >/dev/null 2>&1; then
  echo "ldd is required for the Linux linkage check" >&2
  exit 1
fi

if [ "$#" -eq 0 ]; then
  set -- \
    /ilf/eval_cli \
    /ilf/build_tree_cli \
    /ilf/build_tree_kpmr_cli \
    /ilf/kpmr_source_tree_cli \
    /ilf/src_inherent_cli \
    /ilfx/build/bin/ilfx_cli \
    /ilfx/build/bin/kpmr_cli \
    /ilfx/build/bin/threshold_cli \
    /ilfx/build/bin/rating_threshold_cli \
    /ilfx/build/bin/riskprofile_cli \
    /ilfx/build/bin/riskprofilekpmr_cli \
    /ilfreporter-0.0.1/build/ilfreporter \
    /ilfreporter-0.0.1/build/kpmr_cli \
    /xsltcli/xsltcli
fi

for binary in "$@"; do
  if [ ! -x "$binary" ]; then
    echo "missing executable: $binary" >&2
    exit 1
  fi

  dependencies=$(ldd "$binary")
  if echo "$dependencies" | grep -E 'libopentelemetry[^ ]*\.so|not found' >/dev/null; then
    echo "invalid runtime linkage for $binary" >&2
    echo "$dependencies" >&2
    exit 1
  fi
done

echo "OpenTelemetry linkage check passed for $# executables"
