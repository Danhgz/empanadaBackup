import os
import sys
from csv import reader
import string
import datetime
import time
import logging



#Metodo principal
def main():
    #El nodo naranja actua como un procos que lee cada uno de los archivos csv (misDatos y misVecinos) y los envia
    #A su proceso padre en el nodo verde mediante el uso de un unico pipe IPC.
    #Al usarse un unico pipe debe haber una comunicacion mutua por medio de señales para sincronizar la apertura y cierre del pipe
    #realizada por ambos procesos.
    if os.path.exists('./../logs/logNodoNaranja.tsv'):
        os.remove('./../logs/logNodoNaranja.tsv')
    format = "%(asctime)s\t%(threadName)s\t%(message)s"
    logging.basicConfig(filename='./../logs/logNodoNaranja.tsv', format=format, level=logging.INFO, datefmt="%m/%d/%Y\t%H:%M:%S")
    try:
        pipePath = "./../src/NodoNaranja/fifoNaranja"
        max_size=1024
        try:
            if os.path.exists(pipePath):
                os.remove(pipePath)
                os.mkfifo(pipePath)
                logging.info("Existia un archivo pipe previamente, se borro y se creo uno nuevo.")
            else:
                os.mkfifo(pipePath)
        except:
            logging.info("Ha fallado la creacion del archivo IPC pipe.")
            sys.exit
        logging.info("Se creo correctamente el archivo IPC pipe.")
        while True:
            try:
                pipe_lector = os.open(pipePath, os.O_RDONLY)#se abre el pipe en modo lectura para esperar un mensaje signal del nodo verde
            except:
                logging.info("Error abriendo el extremo de lectura para recibir la señal de nodo verde.")
                sys.exit
            logging.info("Se ha abierto el extremo de lectura para recibir la señal de nodo verde correctamente.")
            bufferSignal = os.read(pipe_lector, max_size)
            signal = bufferSignal.decode()
            signal = signal.rstrip('\x00')
            signal = int(signal)
            try:
                os.close(pipe_lector)
            except:
                logging.info("Error cerrando el extremo de lectura para recibir la señal de nodo verde.")
                sys.exit
            logging.info("Se ha cerrado el extremo de lectura para recibir la señal de nodo verde correctamente.")
            if signal == 1:#nodo verde le indica "quiero recibir mis datos personales", enviandole un 1"
                try:
                    with open('./../misDatos.csv', 'r') as file:
                        archivo_csv = reader(file)
                        for row in archivo_csv:#NN-004: Lectura del archivo
                            try:
                                pipe_escritor = os.open(pipePath, os.O_WRONLY)
                            except:
                                logging.info("Error abriendo el extremo de escritura.")
                                sys.exit
                            logging.info("Se ha abierto el extremo de escritura para enviar misDatos correctamente.")
                            linea=",".join(row)
                            linea_encoded = str.encode(linea + "\0")
                            try:
                                os.write(pipe_escritor, linea_encoded)#NN-005: envio de datos
                                os.close(pipe_escritor)
                            except:
                                logging.info("Error en la escritura de misDatos.csv y cierre del pipe.")
                                archivo_log.write(str(datetime.datetime.now()) + ": Error en la escritura y cierre del pipe.\n")
                                sys.exit
                except:
                    logging.info("Error abriendo el archivo misDatos.csv.")
                    sys.exit
            logging.info("Se enviaron correctamente los datos en misDatos.csv y se cerro el pipe.")
            if signal == 2:#nodo verde le indica "quiero recibir mis datos personales", enviandole un 2"
                try:
                    with open('./../misVecinos.csv', 'r') as file:
                        archivo_csv = reader(file)
                        for row in archivo_csv:#NN-004: Lectura del archivo
                            try:
                                pipe_escritor = os.open(pipePath, os.O_WRONLY)
                            except:
                                logging.info("Error abriendo el extremo de escritura.")
                                sys.exit
                            logging.info("Se ha abierto el extremo de escritura para enviar misVecinos correctamente.")
                            linea=",".join(row)
                            linea_encoded = str.encode(linea + "\0")
                            try:
                                os.write(pipe_escritor, linea_encoded)#NN-005: envio de datos
                                os.close(pipe_escritor)
                            except:
                                logging.info("Error en la escritura de misVecinos.csv y cierre del pipe.")
                                sys.exit
                            while True:
                                try:
                                    pipe_lector = os.open(pipePath, os.O_RDONLY)
                                    bufferSignal = os.read(pipe_lector, max_size)
                                except:
                                    logging.info("Error abriendo y leyendo signal en el pipe.")
                                    sys.exit
                                signal = bufferSignal.decode()
                                signal = signal.rstrip('\x00')
                                signal = int(signal)
                                if signal == 3:#nodo verde le indica "ya termine de recibir esta ronda de datos", enviandole un 3"
                                    os.close(pipe_lector)
                                    break
                except:
                    logging.info("Error abriendo el archivo misVecinos.csv.")
                    sys.exit
                logging.info("Se enviaron correctamente los datos en misVecinos.csv y se cerro el pipe.")
                pipe_escritor = os.open(pipePath, os.O_WRONLY)
                linea_encoded = str.encode("salir\0")
                try:
                    os.write(pipe_escritor, linea_encoded)
                    os.close(pipe_escritor)
                except:
                    logging.info("Error en la escritura de la señal de cierre del programa y cierre del pipe.")
                    sys.exit
                break
        logging.info("Se cerro el programa correctamente.")
    except KeyboardInterrupt:
        loggign.info("Se ha interrumpido la ejecucion del programa.")
        sys.exit

if __name__ == '__main__':
    main()
