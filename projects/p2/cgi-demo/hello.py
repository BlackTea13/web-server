#!/usr/bin/env python3

from os import environ
import cgi, cgitb
import sys

CRLF = '\r\n'

cgitb.enable()

query = cgi.FieldStorage()
name = query.getfirst('name', 'Unknown')

print('HTTP/1.1 200 OK', end=CRLF)
print(f'Server: {environ["SERVER_SOFTWARE"]}', end=CRLF)
print(end=CRLF)

body = ""
for line in sys.stdin:
    body += line

print('<html><body>')
print('<h1>Hello!</h1>')
print(f'<h2>Nice to meet you, {name}!</h2>')
print(f'{body}')
print('</body></html>')
