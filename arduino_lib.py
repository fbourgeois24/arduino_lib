""" Faciliter la communication avec un arduino branché sur le port série 
	Structure des messages envoyés à l'arduino
	'type, addresse, etat'
	Si 	type = 1 -> relais
	 	type = 2 -> pwm
	adresse = numéro de la pin
	etat = 	si > -1 => valeur à écrire (0 ou 1 si digital et entre 0 et 255 pour pwm)
			si = -1 (on demande la lecture), l'arduino répond avec le type, l'adresse et la valeur à la place de l'état
"""

from os import read
import serial
import time
from datetime import datetime as dt
from datetime import timedelta


class arduino():
	def __init__(self, port="/dev/ttyACM0", bitrate=115200, auto_connect = False):
		self.port = port
		self.bitrate = bitrate
		if auto_connect:
			self.connect()
		self.pin_state = [0 for i in range(55)]


	def connect(self):
		""" Ouverture du port série """
		self.arduino = serial.Serial(self.port, self.bitrate, timeout=1)
		time.sleep(1)
		
		if not self.arduino.isOpen():
			raise ConnectionError("Erreur de connexion avec l'arduino")


	def send_message(self, message = "", response_timeout = 5) -> bool:
		""" Envoyer un message à l'arduino
			Dans le cas d'un écriture, renvoie vrai ou faux si la com aboutit ou non
			Dans le cas d'une lecture, renvoie la valeur lue
		"""

		# print(f"Message envoyé à l'arduino : {message}")
		
		# On détecte si c'est un message de read ou write
		if message.split(",")[-1] == "-1":
			message_type = "read"
		else:
			message_type = "write"
		
		# Envoi du message
		message += "\r" # Ajout du retour chariot
		# print(f"Message envoyé à l'arduino : {message}")
		self.arduino.write(message.encode())

		# On attends que l'arduino renvoie le message
		now = dt.now()
		while self.arduino.inWaiting()==0: 
			# Si le timeout est dépassé
			if dt.now() > now + timedelta(seconds=response_timeout):
				print(f"Aucune réponse de l'arduino après {response_timeout} sec")
				return False	
	
		# Si l'arduino a renvoyé un message
		if  self.arduino.inWaiting() > 0: 
			# Lecture et nettoyage de la com
			answer = self.arduino.readline()
			# print(f"Réponse de l'arduino : {answer}")
			self.arduino.flushInput()
			
			# Suivant le type écriture ou lecture, on interprête le résultat
			if message_type == "write":
				# Si la réponse est = au message initial (message correctement envoyé et sortie activée)
				if answer == message[:-1].encode():
					# print("Com Arduino OK")
					return True
				else:
					# print("Com Arduino NOK")
					return False
				
			elif message_type == "read":
				answer = answer[:-1].decode()
				# Suivant le type on renvoie un bool ou une valeur
				if answer.split(",")[0] == "1":
					# si bool
					if answer.split(",")[2] == "0":
						return False
					elif answer.split(",")[2] == "1":
						return True
					else:
						print(f"Erreur de communication, le message '{answer}' renvoyé par l'aruino est invalide")
				elif answer.split(",")[0] == "2":
					try:
						int(answer.split(",")[2])
					except ValueError:
						print(f"Valeur renvoyée par l'arduino incorrecte : {answer}")
					else:
						return int(answer.split(",")[2])
				elif answer.split(",")[0] == "5":
					try:
						float(answer.split(",")[2])
					except ValueError:
						print(f"Valeur renvoyée par l'arduino incorrecte : {answer}")
					else:
						return float(answer.split(",")[2])
				else:
					print(f"Erreur de communication, le message '{answer}' renvoyé par l'aruino est invalide")




	def digitalWrite(self, pin: int, value: bool, verify_response: bool = True, active_low=False) -> bool:
		""" Ecriture d'une sortie TOR sur l'arduino """
		
		# Constitution et envoi du message
		# Inversion de l'état si sortie active bas
		if not active_low:
			if value:
				etat = 1
			else:
				etat = 0
			self.pin_state[pin] = etat
		else:
			if value:
				etat = 0
				self.pin_state[pin] = 1
			else:
				etat = 1
				self.pin_state[pin] = 0
			

		# Update du status de la pin
		

		return self.send_message("1," + str(pin) + "," + str(etat))
		

	def analogWrite(self, pin: int, value: int) -> bool:
		""" Ecriture d'une sortie pwm sur l'arduino """
		if value < 0 or value > 255:
			raise ValueError("Envoi du'une valeur pwm incorrecte vers l'arduino")
			return False

		# Update du status de la pin
		self.pin_state[pin] = value

		return self.send_message("2," + str(pin) + "," + str(value))


	def digitalRead(self, pin: int) -> bool:
		""" Lecture d'une pin TOR sur l'arduino """

		return self.send_message("1," + str(pin) + ",-1")


	def analogRead(self, pin: int) -> int:
		""" Lecture d'une pin analogique sur l'arduino """

		return self.send_message("2," + str(pin) + ",-1")


	def send_config(self, config: dict) -> bool:
		""" Envoyer la config des IO à l'arduino 
			config est un dictionnaire qui contient comme clé le numéro de la pin et comme valeur son type 
			Liste de types : 
			- 0 -> entrée 
			- 1 -> sortie actif haut
			- 2 -> entrée pullup
			- 3 -> sortie actif bas
			- 5 -> DS18#20
		"""

		for pin, pin_type in config.items():
			if self.send_message("0," + str(pin) + "," + str(pin_type)) is False:
					return False

		return True
