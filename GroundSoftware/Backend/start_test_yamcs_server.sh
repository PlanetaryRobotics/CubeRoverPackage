#!/bin/bash

# exit on error
set -eE  # same as: `set -o errexit -o errtrace`

# Some error handling helpers from http://linuxcommand.org/lc3_wss0140.php
PROGNAME=$(basename $0)
function error_exit
{
    echo "${PROGNAME}: ${1:-"Unknown Error"}" 1>&2
    exit 1
}

# This mumbo-jumbo determines the directory containing this shell script, accounting for pretty much any weirdness in
# how this script is invoked (source: https://stackoverflow.com/a/246128). After this snippet, the variable
# THIS_SCRIPT_DIR contains the directory containing this script.
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  THIS_SCRIPT_DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$THIS_SCRIPT_DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
THIS_SCRIPT_DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

# Unless this script was moved (which is unsupported and should be assumed to break this script), it should be in the
# CubeRoverPackage repo. If the root of the CubeRoverPackage repo is "<ROOT>" then we expect this file to be located in
# the directory "<ROOT>/GroundSoftware/Backend". The YAMCS test server directory is expected to be
# "<ROOT>/GroundSoftware/TestYamcsServer". Based on this, we expect this relative path is the correct path to the YAMCS
# test server directory
YAMCS_TEST_SERVER_DIR="${THIS_SCRIPT_DIR}/../TestYamcsServer"

# Enter the YAMCS test server directory
pushd "${YAMCS_TEST_SERVER_DIR}" >/dev/null

# Set up a trap on all errors so that we make sure we call popd before exiting
trap 'popd' ERR

# Run the YAMCS server, this will also compile it as necessary
mvn yamcs:run