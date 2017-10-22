#!/usr/bin/env python
import sys
import struct

class terminator(object):
    terminator = None

    #TODO: Consider passing fmt substr instead of begin/end
    def __init__(self, escape_begin, escape_end):
        self.escape_begin = escape_begin
        self.escape_end = escape_end
    
    def read_arg(self, fmt, stream):
        raise NotImplemented()
    def read_variable_arg(self, fmt, stream):
        raise NotImplemented()

class integer_terminator(terminator):
    terminator = None
    arg_size = None
    is_unsigned = None

    def read_arg(self, fmt, stream):
        l_count = fmt[self.escape_begin + 1: self.escape_end - 1].count('l')
        if l_count <= 1:
            self.arg_size = 4
            self.struct_id = "I" if self.is_unsigned else "i"
        elif l_count == 2:
            self.arg_size = 8
            self.struct_id = "Q" if self.is_unsigned else "q"
        else:
            raise ValueError("Too many l's in format")
        
        arg_data = stream.read(self.arg_size)
        self.value = struct.unpack("<" + self.struct_id, arg_data)[0]

    def read_variable_arg(self, fmt, stream):
        pass

    def __repr__(self):
        return "Integer terminator. Escape {}-{}. Value {}".format(
            self.escape_begin, self.escape_end, self.value)

class d(integer_terminator):
    terminator = "d"
    arg_size = 4
    is_unsigned = False

class u(d):
    terminator = "u"
    is_unsigned = True

class x(d):
    terminator = "x"
    #TODO: Format as hex

class p(d):
    terminator = "p"
    is_unsigned = True
    #TODO: Format as hex

class s(terminator):
    terminator = "s"

    def read_arg(self, fmt, stream):
        self.value = ''

        if self.escape_end - self.escape_begin <= 2:
            self.is_fixed_size = False
            self.fixed_size = 4
            variable_len_data = stream.read(self.fixed_size)
            self.variable_len = struct.unpack("<I", variable_len_data)[0]
        else:
            self.is_fixed_size = True
            self.fixed_size = int(fmt[self.escape_begin + 1: self.escape_end - 1])
            self.value = stream.read(self.fixed_size)
            self.value = self.value.replace('\x00','')

    def read_variable_arg(self, fmt, stream):
        if not self.is_fixed_size:
            self.value = stream.read(self.variable_len)
            self.value = self.value.replace('\x00','')

    def __repr__(self):
        if self.is_fixed_size:
            return 'String terminator. Escape: {}-{}. Fixed size {}. Value {}'.format(
                self.escape_begin, self.escape_end, self.fixed_size, self.value)
        else:
            return 'String terminator. Escape: {}-{}. Fixed size {}. Variable Length {}. Value {}'.format(
                self.escape_begin, self.escape_end, self.fixed_size, self.variable_len, self.value)

terminators = [d, u, x, s, p]

class log_line(object):
    def __init__(self, stream):
        self.stream = stream
        self.line = ""

    def read_fmt(self):
        chars = []
        while True:
            c = self.stream.read(1)
            if not c:
                raise StopIteration()
            if c == '\0':
                break
            chars.append(c)
        fmt = ''.join(chars)
        return fmt

    def parse_fmt(self, fmt):
        terminator_list = []
        is_escaped = False
        escape_begin = -1
        for i, c in enumerate(fmt):
            if c == '%':
                if is_escaped and escape_begin == i - 1:
                    is_escaped = False
                    escape_begin = -1
                else:
                    is_escaped = True
                    escape_begin = i
                continue
            
            if not is_escaped:
                continue
            
            candidates = filter(lambda x: x.terminator == c, terminators)
            if not candidates:
                continue
            if len(candidates) > 1:
                raise ValueError("More than one terminator match this character: {}".format(c))
            term = candidates[0](escape_begin, i + 1)
            terminator_list.append(term)

            is_escaped = False
            escape_begin = -1

        return terminator_list

    def parse(self):
        fmt = self.read_fmt()
        terminator_list = self.parse_fmt(fmt)
        for term in terminator_list:
            term.read_arg(fmt, self.stream)
        
        for term in terminator_list:
            term.read_variable_arg(fmt, self.stream)
            
        line_parts = []
        last_escape_end = 0
        for term in terminator_list:
            prev_fmt_part = fmt[last_escape_end: term.escape_begin]
            line_parts.append(prev_fmt_part)

            line_parts.append('{}'.format(term.value))

            last_escape_end = term.escape_end
        
        last_fmt_part = fmt[last_escape_end:]
        line_parts.append(last_fmt_part)

        self.line = ''.join(line_parts)

    def __str__(self):
        return self.line
            


class llcpp_parser(object):
    def __init__(self, stream):
        self.stream = stream

    def lines(self):
        while True:
            line = log_line(self.stream)
            line.parse()
            yield str(line)

def main(argv):
    parser = llcpp_parser(open(argv[1],'rb'))
    for line in parser.lines():
        sys.stdout.write(line)

if __name__ == '__main__':
    main(sys.argv)