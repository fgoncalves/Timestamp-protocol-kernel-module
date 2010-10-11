import re
import sys

done = dict()

if __name__ == '__main__':
    pat = re.compile('(\d+)[ \t]+\d+[ \t]+(\d+).')
    first = False
    p = 1.0/100.0 * 1000000000.0
    for line in open(sys.argv[1]):
        match = pat.match(line)
        if match:
            if not first:
                first = True
                id_first = int(match.group(2))
                time_first = int(match.group(1))
            try:
                done[str(match.group(2))]
            except KeyError:
                done[str(match.group(2))] = True
                print float(match.group(1)) - (p*(int(match.group(2)) - id_first) + time_first)
            
