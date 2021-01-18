# TTWatch Companion Server
# This server allows the Twatch2020 or any other web client to remote control the PC running the server and its periphaerals

# NOTE
# A certificate must be generated using keygen before running the server - could automate later

# TODO 
# Near-term:
# Add compatability functions for use on a raspberry pi
# Clean up arg parsing in POST function
# Add key pair generation instead of using passwords

# Long-term:
# Add extended features to GET or even allow the server to make POST requests back to the watch to update variables internal to the watch.
# Add a UI for adding functions (not disimilar from unified remote)
# Add the ability to upload roms through web interface
# Automate keygen

# Server Imports
import http.server
import socketserver
from urllib.parse import urlparse, parse_qs
import ssl

# Regex
import re

# Operating System Interfacing
import os
import ctypes
from subprocess import call

from io import BytesIO

# Web Browser Control
import webbrowser

# Folder Locations
git_folder = '"C:\\Users\\Lenovo\\Files\\git"'
auto_git_script = '"C:\\Users\\Lenovo\\Files\\git\\Windows-Scripts\\Git_Scripts\\Auto-git.ps1"'

# Server Setup
PORT = 8000

# HTTP Request Class
class MyHttpRequestHandler(http.server.SimpleHTTPRequestHandler):

	PORT = 8000
	SEC_KEY = "1010" # This key is rudimentary access control to be substituted with HTTPS for encryption & a further access keypair for both the server and client

	def do_GET(self): # Parses GET requests

		query_components = parse_qs(urlparse(self.path).query)
		print(query_components)

		regex='/\?.*key='+MyHttpRequestHandler.SEC_KEY+".*"

		if re.match(regex, self.path) is not None:

			try:
				self.command_switch(query_components["command"][0], query_components["argument"][0])
			except:
				pass

			self.path = 'index.html'
			return http.server.SimpleHTTPRequestHandler.do_GET(self)
		else: # if path did not contain key
			return

	def do_POST(self): # Parses POST Requests
	#NOTE - the data parsing portion of this code is a hot mess. It needs unifying so that instead of the two types of input being processed in its own way 
	# the data is pulled, converted to the same format e.g. a string, then checked for bad input. after that the string can be spilt into a dictonary.
	# Note that data pulled from parse_qs is encapsulated in a list so needs to be called with a [0].
		
		query_components = parse_qs(urlparse(self.path).query)# Tries to acquire url arguments the normal way
		
		# If query_components empty, fallback, try to acquire them directly from the input stream
		if (len(query_components) <= 1):

			content_length = int(self.headers['Content-Length']) # <--- Gets the size of data
			post_data = self.rfile.read(content_length).decode("utf-8") # <--- Gets the data itself
			
			#Filters user input (Security layer) - needs 
			data = []
			for character in post_data:
				if character.isalnum():
					data.append(character)
				elif character == '&':
					data.append(character)
				elif character == '=':
					data.append(character)
			data = ''.join(data)

            #Converts inputstream data to a dictonary
			data = data.split('&') # <--- Split the string on the &, outputs key value pairs as a 1D list of strings
			for i in data : # Recurse over each stringified key, value pair
				i = i.split('=') # <--- Split the string on the eqals sign, outputs a list containing one key and one value
				print(i[0]+": "+i[1]) # <--- Show arguments for current request in terminal
				query_components[i[0]] = i[1] # <--- Assigns the key and value from the list object to the query_components dictionary

		# If query_components had a value to begin with, the client has sucsessfully formatted 
		# our path arguments for the server and the server has parsed it correctly
		else: 
			for k, v in query_components.items():

				try: # Remove any encapsulating list
					query_components[k] = v[0]
				except:
					query_components[k] = v
				finally: # Show arguments for current request in terminal
					print(k+": "+query_components[k])

		try:
			key = query_components["key"]
			print(key)
		except:
			return
		else:
			if key == MyHttpRequestHandler.SEC_KEY:
				# try:
				self.command_switch(query_components["command"], query_components["argument"])
				# except:
				# 	pass
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

	# def send_error(self, code, message=None):

	# 	if code == 404:
	# 		self.error_message_format = "Does not compute! 404"
	# 	if code == 301:
	# 		self.error_message_format = "Does not compute! 301"
	# 	if code == 500:
	# 		self.error_message_format = "Does not compute! 500"

	# 	return SimpleHTTPRequestHandler.send_error(self, code, message)


	def process_launcher(self, process_type, c, argument):
		# Creates a process and records its PID so it can be killed later on	
		print(c)
		if process_type == "python":
			call(["python", c])
		elif process_type == "powershell":
			os.system(c) 
		elif process_type == "bookmark":
			webbrowser.open(c, new=2)
		elif process_type == "none":
			exec (c)

	def command_switch(self, command, argument):
		#Parses command arguments
		print("com "+command)
		if command == "Settings": 
			self.process_launcher("bookmark", "https://127.0.0.1:"+str(MyHttpRequestHandler.PORT)+"/?key="+MyHttpRequestHandler.SEC_KEY, None)
			webbrowser.open("https://127.0.0.1:"+str(MyHttpRequestHandler.PORT)+"/?key="+MyHttpRequestHandler.SEC_KEY, new=2)
		elif command == "Backup": 
			print("System backup selected")
		
		elif command == "GitUpload": 
			self.process_launcher("powershell", "cd "+git_folder+" ; & "+auto_git_script, "")
		
		elif command == "Trade": 
			print("Trading mode selected")
		
		elif command == "Program": 
			print("Programming mode selected")
		
		elif command == "Web Admin": 
			print("Web Admin mode selected")
		
		elif command == "Lock": 
			ctypes.windll.user32.LockWorkStation()
			
		elif command == "Shutdown": 
			self.process_launcher("powershell", "shutdown /s /t 1", "")
		
		elif command == "Harden": 
			print("Harden selected")
		
		elif command == "Obfuscate": 
			print("Obfuscate selected")
		
		elif command == "Monitor": 
			print("Monitor selected")
		
		elif command == "AutoLock": 
			self.process_launcher("python", "C:\\Users\\Lenovo\\Files\\git\\facial-recognition-logout\\Facial_Recognition_V4.py", "")
		
		elif command == "RestartServer":
			print("Restarting server")
		
		else:
			print("Command not recognised")


	def security_strip(post_data):
		data = []
		for character in post_data:
			if character.isalnum():
				data.append(character)
			elif character == '&':
				data.append(character)
			elif character == '=':
				data.append(character)
		data = ''.join(data)


	def sanity_check():
		#checks process being run against an encrypted hash
		pass


	def key_pair_generate():
		os.system("openssl req -new -x509 -keyout yourpemfile.pem -out yourpemfile.pem -days 365 -nodes")

# Create an object of the above class
handler_object = MyHttpRequestHandler

my_server = socketserver.TCPServer(("0.0.0.0", PORT), handler_object)
# Wraps socket for HTTPS
my_server.socket = ssl.wrap_socket (my_server.socket, certfile='./yourpemfile.pem',  server_side=True)
# Start the server
my_server.serve_forever()