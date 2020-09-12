

//--------------------------Declaración de Funciones------------------------------
/* Formato:
*Al inicio del nombre de un meto se identifica una letra:
*  - "h" para metodos correspondientes a hilos. Ejemplo: h_instanciarNodoNaranja.
*  - "u" de utilidad, métodos que utilizan los hilos. Ejemplo:  u_insertarIPVecinos.
* Esta va seguida de otra letra que corresponde al agente con el que trabaja esta funcion:
* - "v" para el nodo verde
* - "n" para el nodo naranja
* - "a" para el nodo azul
* - "g" para un metodo de uso general
*/

//Métodos del Nodo verde
void uv_EstablecerConexionCliente(int puerto_destino, std::string ip_destino, int pos);
void uv_EstablecerServidor();
int uv_ObtenerCantidadVecinos();
std::string uv_ObtenerIPVecino(int posicion);
int uv_ObtenerPuertoVecino(int posicion);
void uv_InsertarPaqueteEntrada(std::string paqueteEntrada);
std::string uv_ObtenerPaqueteSalida();
void uv_ObtenerIDVecino(int posicion);

//Métodos del Nodo Naranja
void pn_IntanciarNodoNaranja();
void un_InsertarIPVecinos(std::pair<int,std::string>id_IP);
void un_InsertarPuertoVecinos(std::pair<int,int>id_puerto);

//Métodos del Nodo Azul
void ha_InstanciarNodoAzul();
void ha_InstanciarAgenteAzulEmisor();
void ha_InstanciarNodoAzulReceptor();
void ua_InsertarPaqueteSalida(std::string paqueteSalida);
std::string ua_ObtenerPaqueteEntrada();

//Métodos Generales
std::string ug_convertirString(char* arr, int tamano);
void logger(std::string modulo,std::string message);
//------------------------------------------------------------------------------

//--------------------------Declaración de Variables----------------------------
int miID;                             //número que identifica el nodo verde.
int miPuerto;
bool finalizar;                         //número que identifica el puerto de comunicación.
std::string miIP;                          //dirección IP del nodo verde.
std::vector< std::pair<int, std::string> > vecinos_id_ip;       //mapa que contiene el ID y la dirección IP de cada vecino.
std::vector< std::pair<int, int> > vecinos_id_puerto;      //mapa que contiene el ID y puerto de cada vecino.
std::queue<std::string> entrada;
std::queue<std::string> salida;
sem_t semNaranja;
std::vector<sem_t> semClientes;
std::map<int,int> clientes_id_indice;
std::mutex candadoColaSaliente, candadoColaEntrante;



//------------------------------------------------------------------------------
