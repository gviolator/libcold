#! /bin/bash

Python=$(which python3)
if [ -z $Python ]; then
	echo 'no python3'
	exit 1
fi


BuildscriptsPath=$(dirname $(readlink -f $0))
BuildscriptsPath=$(dirname $BuildscriptsPath)

export PATH=~/llvm/12.0/bin:$PATH

# PythonEnvPath=$BuildscriptsPath/.venv

# if [ ! -d "$PythonEnvPath" ]; then
# 	echo "setup python env ($PythonEnvPath) by ($Python)"
# 	$Python -m venv $PythonEnvPath
# fi

# source "$PythonEnvPath/bin/activate"
# pip install --requirement $BuildscriptsPath/requirements.txt



# echo "dirname/readlink: $(dirname $(readlink -f $0))"

# PYTHON=$(which python3)
# if [ -z $PYTHON ]; then
# 	echo 'no python3'
# 	exit 1
# fi



# echo "It here = $PYTHON"