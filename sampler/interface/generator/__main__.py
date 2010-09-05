import sys
import parser
import page
import javascripts
from slot import Slot

if __name__ == '__main__':
    slots = dict()
    
    if len(sys.argv) == 2:
        f = open(sys.argv[1], 'r')
        if f:
            for line in f:
                parsed = parser.parse_line(line)
                if parsed:
                    try:
                        sl = slots[parsed['source']]
                        sl.packet_delay_data.append(parsed['timestamp'])
                        sl.packet_delay_variation_data.append(parsed['delay'])
                        sl.packet_data.append(parsed['data'])
                    except KeyError:
                        sl = Slot(parsed['source'])
                        sl.packet_delay_data.append(parsed['timestamp'])
                        sl.packet_delay_variation_data.append(parsed['delay'])
                        sl.packet_data.append(parsed['data'])
                        slots[parsed['source']] = sl
            html_out = open('results.html', 'w')
            scripts_out = open('scripts/analise.js', 'w')
            html_out.write(page.new_html_page(* slots.values()))
            scripts_out.write(javascripts.generate_scripts(* slots.values()))
            html_out.close()
            scripts_out.close()
            f.close()
        else:
            print 'Unable to open file ' + sys.argv[1]
    else:
        print 'usage: python ' + sys.argv[0] + ' <statistics file>'
