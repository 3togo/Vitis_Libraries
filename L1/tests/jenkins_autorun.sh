#/bin/bash

TEST_DIR=./out_test
STAT=./statistics.rpt

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
  mkdir $TEST_DIR
fi

if [ -f $STAT ]; then
  rm $STAT -rf
fi

source /group/xsjfarm/lsf/conf/profile.lsf
source set_env.sh

PYTHON=python3
PYTEST=./sw/python/run_test.py
PYCHECK=./sw/python/check_process.py

PAR=8
id=0

SUBMIT="bsub -cwd `pwd` -q medium -R \"select[(os== lin && type == X86_64 && (osdistro == rhel || osdistro == centos) && (osver== ws7))] rusage[mem=16000]\""

OPLIST=("amax amin asum axpy copy dot nrm2 scal swap" \
    "gemv gbmv sbmvLo sbmvUp tbmvLo tbmvUp trmvLo trmvUp" \
    "symvLo symvUp spmvUp spmvLo tpmvLo tpmvUp --csim")

for val in "${OPLIST[@]}"; do
  eval "$SUBMIT $PYTHON $PYTEST --operator $val --parallel $PAR --id $id"
  ((id++))
done

$PYTHON $PYCHECK --number $id

if [ -f $STAT ]; then
  cat $STAT
fi
