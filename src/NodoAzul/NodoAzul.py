import os							#os.path,os.mkfifo,os.open,os.read,os.write.
import sys							#sys.exit.
import threading					#Manejo de hilos.
import time							#Configuracion de logs timefmt.
import logging 						#Manejo de logs.
import tkinter as tk				#Interfaz grafica.
from functools import partial		#Llamado de metodos desde interfaz grafica.

#Variables globales
MAX_SIZE = 1024
miID = ""
bandera = True

""" Hilo de envio de paquetes extraidos de la interfaz grafica al agente azul.

\details Hilo del nodo azul encargado de crear el pipe de envio, interfaz de usuario y
empaquetado de mensajes y envio de paquetes por el pipe emisor.
\param idNodo: Identificador simple para cada nodo.
"""
def p_InstanciarHiloEmisor(idNodo):
	pipePath = "./IPC/NodoAzul/fifoEmisorAzul"
	try:
		if os.path.exists(pipePath):
			os.remove(pipePath)
			os.mkfifo(pipePath)
		else:
			os.mkfifo(pipePath)
		logging.info("Se ha creado el IPC pipe de escritura.")
	except:
		logging.error("Ha fallado la creacion del archivo IPC pipe.")
		sys.exit
	root = tk.Tk()
	root.geometry('400x200+100+200')
	root.title('Ingrese ID destino y mensaje')
	mensaje1 = tk.StringVar()
	mensaje2 = tk.StringVar()
	labelData1 = tk.Label(root, text="ID").grid(row=1, column=0)
	labelData2 = tk.Label(root, text="Mensaje").grid(row=2, column=0)
	entryData1 = tk.Entry(root, textvariable=mensaje1).grid(row=1, column=2)
	entryData2 = tk.Entry(root, textvariable=mensaje2).grid(row=2, column=2)
	ButtonSend = tk.Button(root, text="Enviar", command=partial(envioDePaquete,pipePath,mensaje1,mensaje2)).grid(row=3, column=2)
	root.mainloop()#despues de este punto el programa NO sigue hasta que la ventana se cierra
    #se cierra ventana, flag de fin "gracioso" del programa=true (no es prioridad en esta fase) y se devuelve al main

""" Metodo de empaquetado y envio de paquetes al agente azul.

\details Metodo utilitario del hilo emisor para crear los paquetes y enviarlos
por el pipe de salida.
\param pipePath: Ruta del pipe emisor.
\param mensaje1: Id del nodo destino especificado por el usuario.
\param mensaje2: Mensaje a enviar de 200B maximo.
"""
def envioDePaquete(pipePath, mensaje1, mensaje2):
	id = mensaje1.get().zfill(2)[-2:] #Garantiza que ID mida 2 bytes.
	mensaje = mensaje2.get()[:200] #Maximo 200 bytes de mensaje.
	paquete = miID + id + mensaje #miID ya esta en formato 2 bytes.
	print(paquete)
	try:
		pipe_escritor = os.open(pipePath, os.O_WRONLY)
		logging.info("Pipe de envio abierto exitosamente.")
	except:
		logging.info("Error abriendo el extremo de escritura.")
		sys.exit
	try:
		os.write(pipe_escritor, paquete.encode())
		os.close(pipe_escritor)
	except:
		logging.info("Error en la escritura y cierre del pipe.")
		sys.exit
	try: #Recibe signal para coordinacion
		pipe_lector = os.open(pipePath, os.O_RDONLY)
	except:
		logging.info("Error abriendo pipe de envio.")
		sys.exit
	try:
		bufferSignal = os.read(pipe_lector, MAX_SIZE) #Signal para coordinacion
		os.close(pipe_lector)
	except:
		logging.info("Error leyendo signal y cerrando el pipe.")
		sys.exit



""" Hilo que despliega paquetes recibidos del agente azul.

\detaisl Hilo encargado del recibo de paquetes del nodo verde por el
pipe receptor, desempaquetado de mensajes y despliegue en consola de los mismos.
\param idNodo: Identificador simple para cada nodo.
"""
def p_InstanciarHiloReceptor(idNodo):
	pipePath = "./IPC/NodoAzul/fifoReceptorAzul"
	try: #Creacion del pipe receptor
		if os.path.exists(pipePath):
			os.remove(pipePath)
			os.mkfifo(pipePath)
		else:
			os.mkfifo(pipePath)
		logging.info("Se ha creado el IPC pipe de lectura.")
	except:
		logging.error("Ha fallado la creacion del archivo IPC pipe.")
		sys.exit
	#Recepcion de mi ID del nodo verde para empaquetado de mensajes.
	try:
		pipe_lector = os.open(pipePath, os.O_RDONLY)#se abre el pipe en modo lectura para esperar un mensaje signal del agente azul
		logging.info("Se ha abierto el extremo de lectura correctamente.")
	except:
		logging.error("Error abriendo el extremo de lectura.")
		sys.exit
	try:
		id = os.read(pipe_lector, MAX_SIZE)
		os.close(pipe_lector)
		logging.info("Se ha cerrado el extremo de lectura correctamente.")
		global miID
		miID = id.decode().zfill(2)[-2:]
		print("\n Mi ID: " + miID)
	except:
		logging.error("Error cerrando el extremo de lectura.")
		sys.exit
	time.sleep(1.1)
	while bandera: #Recepcion de mensajes para su despliegue.
		try:
			pipe_lector = os.open(pipePath, os.O_RDONLY)
		except:
			logging.error("Error abriendo pipe de lectura.")
			sys.exit
		paquete = os.read(pipe_lector, MAX_SIZE)
		paquete = paquete.decode()
		logging.info(paquete[2:4]+" dice: "+ paquete[4:])
		print("* "+ paquete[:2] +" dice: "+ paquete[4:])
		try: #Envio de signal para coordinacion.
			pipe_escritor = os.open(pipePath, os.O_WRONLY)
		except:
			logging.error("Error abriendo el extremo de escritura.")
			sys.exit
		try:
			os.write(pipe_escritor, "signal".encode())#envia signal para coordinacion.
			os.close(pipe_escritor)
		except:
			logging.error("Error en la escritura del signal y cierre del pipe.")
			sys.exit

#Metodo principal
def main():
	logPath = './../logs/logNodoAzul' +  '.tsv'
	if os.path.exists(logPath):
		os.remove(logPath)
	format = "%(asctime)s.%(msecs)03d\t%(levelname)s\t%(message)s"
	logging.basicConfig(filename=logPath, format=format, level=logging.INFO, datefmt="%Y-%m-%d %H:%M:%S")

	#se inicializan los threads
	thread_1 = threading.Thread(target=p_InstanciarHiloEmisor, args=("1",))
	thread_2 = threading.Thread(target=p_InstanciarHiloReceptor, args=("2",))

	thread_1.start()
	thread_2.start()

	global bandera
	thread_1.join()
	print("Finalizado envio de mensajes!")
	bandera = False #Señal para finalizar hilo
	thread_2.join() #Necesita que entre un ultimo mensaje.
	print("Finalizada recepcion de mensajes!")

if __name__ == '__main__':
	main()
