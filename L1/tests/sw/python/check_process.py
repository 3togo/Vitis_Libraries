import time
import functools
import argparse
import os, sys
import re
import subprocess, shlex
import pdb

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])


class Jobs:
  def __init__(self, jobList):
    self.jobIds = list()
    self.statList = list()
    self.size = 0
    pattern = "Job <(\d+)> is submitted"
    for i in range(len(jobList)):
      file = jobList[i]
      with open(file, 'r') as f:
        content = f.read()
        match = re.search(pattern, content)
        if not match is None:
          id = match.group(1)
          self.jobIds.append(id)
          self.statList.append('statistics_%d.rpt'%i)
          self.size +=1

  def alive(self, id):
    commandLine = "bjobs %s"%id
    args = shlex.split(commandLine)
    result = subprocess.check_output(args).decode("utf-8")
    pattern = "not found"
    match = re.search(pattern, result)
    if match is None:
      return True
    else:
      return False

  def aliveAny(self):
    a = [self.alive(id) for id in self.jobIds]
    func = lambda x, y : x or y
    return functools.reduce(func, a)

  def checks(self):
    fileFound = [os.path.exists(f) for f in self.statList]
    func = lambda x, y : x and y
    return functools.reduce(func, fileFound)

def poll(jobs, t, id_max, progress = 80):
  id = 0
  alive = jobs.aliveAny()
  while id < id_max or alive:
    if alive:
      id_max+=1
    else:
      if jobs.checks():
        print("Polling finished.")
        break
    id+=1
    print("Sleeping for %ds."%(t))
    perT = t / progress
    sys.stdout.write('%s: [='%Format(id))
    for i in range(progress):
      sys.stdout.write('\b=%d'%(i%10))
      sys.stdout.flush()
      time.sleep(perT)
    sys.stdout.write('\b]\n')
    alive = jobs.aliveAny()
  return id == id_max


def merge(fileList, filename):
  with open(filename, 'w+') as f:
    for file in fileList:
      with open(file, 'r+') as fr:
        f.write(fr.read())

def main(args): 

  jobs = Jobs(['%s_%d'%('job', i) for i in range(args.number)])
  if jobs.size == 0:
    return
  if poll(jobs, args.time, args.timeout/args.time):
    print("Time out, please check the logfiles.")
  else:
    merge(fileList, '%s.%s'%(args.basename,args.ext))

if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--number', type=int, required=True, help='number of files')
  parser.add_argument('--time', type=int, default=60, help='number of seconds to poll')
  parser.add_argument('--timeout', type=int, default=12000, help='number of seconds to time out')
  args = parser.parse_args()
  
  main(args)

