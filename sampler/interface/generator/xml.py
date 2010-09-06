def element(element_name, element_text = None, **element_attrs):
    res = '<' + element_name
    for key in element_attrs:
        if key.endswith('_'):
            res += ' ' + key[:-1] + '="' + element_attrs[key] + '"'
        else:
            res += ' ' + key + '="' + element_attrs[key] + '"'
    res += '>'
    if element_text:
        res += element_text
    res += '</' + element_name + '>'
    return res
