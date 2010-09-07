def new_line_graph(title, legend, data):
    script = "jQuery('<img>').attr('src', api.make({\n\tdata: ["
    if len(data) > 0:
        script += str(data[0])
        for i in data[1:]:
            script += ", " + str(i)
    script += "],\n\tsize: '800x250',\n\ttype: 'lc',\n\ttitle: '" + title + "',\n\tlegend: ['" + legend + "']}))"
    return script

def new_packet_delay_graph(slot):
    script = new_line_graph("Packet Arrival Time", 'Time at which each packet arrived', slot.packet_delay_data) + '.prependTo("#' + slot.source.replace('.', '_') + '-packet-delay");'
    return script


def new_packet_data_graph(slot):
    script = new_line_graph("Collected Data", 'Data collected by node ' + slot.source, slot.packet_data) + '.prependTo("#' + slot.source.replace('.', '_') + '-packet-data");'
    return script

def new_slot_script(slot):
    script = 'function execute' + slot.source.replace('.','_') + "(){\n"
    script += "var api = new jGCharts.Api();\n"
    script += new_packet_data_graph(slot) + "\n" + new_packet_delay_graph(slot) + "\n}\n"
    return script

def generate_scripts(*slot_list):
    script = ''
    for sl in slot_list:
        script += new_slot_script(sl)
    return script
