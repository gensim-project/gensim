from argparse import ArgumentParser
import sys
import subprocess
from select import select
import time

# Wait for the given line to be output by the process.
def wait_for_line(process, needle, max_timeout=1000):
	start = time.clock()
	while(time.clock() - start < max_timeout):
		process.poll()
		if(process.returncode != None):
			return False
		
		result = select([process.stdout], [], [], 1)
			
		# Do we have any stdout available?
		if(result[0]):
			# We have some stdout available, so read it line by line until we don't have any left
			line = process.stdout.readline().decode('ascii')
			if(line == ''):
				continue
				
			print(line)
			
			if(line.find(needle) == 0):
				# We found it!
				return True
				
	return False

def find_archsim_binary():
	process = subprocess.Popen("hg root", shell=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
	return process.stdout.readline().decode('ascii').strip('\n').strip('\r') + "/build/dist/bin/archsim"
	
def main():
	parser = ArgumentParser()
	parser.add_argument("-e", "--zimage", dest="zimage")
	parser.add_argument("-v", "--verbose", dest="verbose", type=int, choices=[1,0])
	command_line_args = parser.parse_args()
	
	args='virtio_mmio.device=1K@0x10200000:34 earlyprintk=serial console=ttyAMA0 root=/dev/vda1 rw norandmaps verbose text'
	archsim=find_archsim_binary()
	model_flags='-s armv7a -m arm-realview -l contiguous --sys-model base '
	zimage = command_line_args.zimage
	kernel_flags='--bdev-file /dev/null --binary-format zimage -e ' + zimage + ' --kernel-args "' + args + '"'
	verbose_flags = ''
	
	if(command_line_args.verbose):
		verbose_flags = '--verbose'

	# If we use Popen with shell=True and then try and kill the process,
	# the shell will be killed but the process will live on. So, use 'exec'
	# (which replaces the shell with the child process) to avoid this.
	command="exec " + archsim + " " + model_flags + " " + kernel_flags 

	final_line = '---[ end Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)'
	process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
	result = wait_for_line(process, final_line, 30)
	
	exitcode = 0
	
	if(result):
		print("Success!")
		exitcode = 0
	else:
		print("Failure!")
		exitcode = 1
	
	process.kill()
	return exitcode

if __name__ == "__main__":
    sys.exit(main())
