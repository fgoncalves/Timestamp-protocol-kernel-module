import re
import time
import calendar
import argparse
import sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser = argparse.ArgumentParser(description='Use this script to convert the output of these tests to the input of the script deviation.py.')
    parser.add_argument('-f', '--file', dest='file', metavar='FILE_NAME', nargs=1, help='Convert FILE_NAME to the expected input for deviation.py.', required=True)

    args = parser.parse_args()
    
    f = open(args.file[0])
    if f:
        #prefix string so it can convert the time correctly.
        prefix = '1970-01-01-'
        sys.stderr.write("Warning: Using date prefix: " + prefix + "\n")

        pattern = re.compile('(\d+:\d+:\d+):(\d+)(.*)')
        for line in f:
            parsed = pattern.match(line)
            if parsed:
                t = time.strptime(prefix + parsed.group(1), '%Y-%m-%d-%H:%M:%S')
                epoch_nanosecs = calendar.timegm(t)*1000000000 + int(parsed.group(2)) * 1000000
                print str(epoch_nanosecs) + parsed.group(3)
        f.close()
    else:
        sys.stderr.write('Unnable to open file ' + args.f + "\n")
