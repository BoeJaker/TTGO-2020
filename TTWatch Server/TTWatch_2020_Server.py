import http.server
import socketserver
from urllib.parse import urlparse, parse_qs
import ssl

import re

import os
import ctypes
from subprocess import call

from io import BytesIO

import webbrowser

git_folder = '"C:\\Users\\Lenovo\\Files\\git"'
auto_git_script = '"C:\\Users\\Lenovo\\Files\\git\\Windows-Scripts\\Git_Scripts\\Auto-git.ps1"'

PORT = 8000

class MyHttpRequestHandler(http.server.SimpleHTTPRequestHandler):

	PORT = 8000
	SEC_KEY = "1010"

	def do_GET(self): # Parses GET requests

		query_components = parse_qs(urlparse(self.path).query)
		print(query_components)

		try:
			key = query_components["key"]
		except:
			key=""

		regex='/\?.*key='+MyHttpRequestHandler.SEC_KEY+"?&.*"

		if re.match(regex,  self.path) is not None:
			self.command_switch(query_components["command"][0], query_components["argument"][0])
			self.path = 'index.html'
			return http.server.SimpleHTTPRequestHandler.do_GET(self)
		else: # if path did not contain key
			return


	def do_POST(self): # Parses POST Requests

		content_length = int(self.headers['Content-Length']) # <--- Gets the size of data
		post_data = self.rfile.read(content_length) # <--- Gets the data itself
		print(post_data)

		query_components = parse_qs(urlparse(self.path).query)
		print(query_components)

		try:
			key = query_components["key"][0]
			print(key)
		except:
			key = ""

		if key == MyHttpRequestHandler.SEC_KEY:

			self.command_switch(query_components["command"][0], query_components["argument"][0])
			
			content_length = int(self.headers['Content-Length'])
			body = self.rfile.read(content_length)
			self.send_response(200)
			self.end_headers()
			response = BytesIO()
			response.write(b'This is a POST request. ')
			response.write(b'Received: ')
			response.write(body)
			self.wfile.write(response.getvalue())
		else: # if key does not match
			return

	def send_error(self, code, message=None):

		if code == 404:
			self.error_message_format = "Does not compute! 404"
		if code == 301:
			self.error_message_format = "Does not compute! 301"
		if code == 500:
			self.error_message_format = "Does not compute! 500"

		return SimpleHTTPRequestHandler.send_error(self, code, message)


	def command_switch(self, command, argument):
		#Parses command arguments
		if command == "Settings": 
			webbrowser.open("http://127.0.0.1:"+str(MyHttpRequestHandler.PORT)+"/?key="+MyHttpRequestHandler.SEC_KEY, new=2)
		
		elif command == "Backup": 
			print("System backup selected")
		
		elif command == "Git Upload": 
			os.system("cd "+git_folder+" ; & "+auto_git_script)
		
		elif command == "Trade": 
			print("Trading mode selected")
		
		elif command == "Program": 
			print("Programming mode selected")
		
		elif command == "Web Admin": 
			print("Web Admin mode selected")
		
		elif command == "Lock": 
			ctypes.windll.user32.LockWorkStation() 
		
		elif command == "Shutdown": 
			os.system("shutdown /s /t 1") 
		
		elif command == "Harden": 
			print("Harden selected")
		
		elif command == "Obfuscate": 
			print("Obfuscate selected")
		
		elif command == "Monitor": 
			print("Monitor selected")
		
		elif command == "Facial_Recognition_Lock": 
			call(["python", "C:\\Users\\Lenovo\\Files\\git\\facial-recognition-logout\\Facial_Recognition_V4.py"])
		
		elif command == "Restart Server":
			print("Restarting server")
		
		else:
			print("Command not recognised")


	def start_process():
		# Creates a process and records its PID so it cna be killed later on	
		pass


	def security_strip():
		# Removes all special chars from human input, leaving just alphanumerical operators - reducing attack surface
		pass


	def sanity_check():
		#checks process being run against an encrypted hash
		pass



# Create an object of the above class
handler_object = MyHttpRequestHandler

my_server = socketserver.TCPServer(("0.0.0.0", PORT), handler_object)
# my_server.socket = ssl.wrap_socket (my_server.socket, certfile='./cert.pem', keyfile='./key.pem', server_side=True)
# Star the server
my_server.serve_forever()