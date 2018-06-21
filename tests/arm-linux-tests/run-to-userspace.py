import sys
import subprocess
from select import select
import time

# Wait for the given line to be output by the process.
def wait_for_line(process, needle, max_timeout=1000):
	while(max_timeout > 0):
		before_select = time.clock()
		result = select([process.stdout], [], [], 1)
		after_select = time.clock()
		
		max_timeout -= after_select - before_select
		
		# Do we have any stdout available?
		if(result[0]):
			# We have some stdout available, so read it line by line until we don't have any left
			while(True):
				result = select([process.stdout],[],[])
				if(result[0]):
					line = process.stdout.readline()
					
					print line
					
					if(line.find(needle) == 0):
						# We found it!
						return True
					
				else:
					break
	return False

args='virtio_mmio.device=1K@0x10200000:34 earlyprintk=serial console=ttyAMA0 root=/dev/vda1 rw norandmaps verbose text'
archsim='../../build/dist/bin/archsim'
model_flags='-s armv7a -m arm-realview -l contiguous --sys-model base '
kernel_flags='--bdev-file /dev/null --binary-format zimage -e linux-4.3/arch/arm/boot/zImage --kernel-args "' + args + '"'

command=archsim + " " + model_flags + " " + kernel_flags 

def main():
	final_line = '---[ end Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)'
	process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
	try:
		result = wait_for_line(process, final_line, 30)

		if(result):
			print "Success!"
			return 0
		else:
			print "Failure!"
			return 1
	finally:
		process.kill()


if __name__ == "__main__":
    sys.exit(main())
