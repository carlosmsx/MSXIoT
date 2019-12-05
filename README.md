#MSX IoT

Internet de las cosas para MSX
Si estás en este sitio probablemente ya sabes lo que es un MSX.
Pero, has oído hablar de Internet de las cosas (Internet of Things)?
Te presento MSX IoT.


Es un cartucho para MSX que permite IoT y algunos extras muy interesantes:

Conectarse a redes WiFi
Conectarse a dispositivos BT
Conectarse a dispositivos seriales (TTL)
Audio
Disco en la nube
Unidades de red

Una vez conectado a una red WiFi, las posibilidades son muchas. Por ejemplo, es posible comandar dispositivos a través de internet, o bien saber el estado de los mismos.

A nivel de hardware, MSX IoT consta de una ROM, un decodificador de direcciones y un módulo ESP32.

Cada vez que el Z80 accede para lectura o escritura a un puerto designado en el decodificador, se genera una señal WAIT hacia el Z80 y paralelamente se lanza una interrupción en el ESP32. Esto permite que en el microcontrolador se compruebe si la interrupción se debe a una operación de entrada o salida. Con esto el ESP32 puede leer bytes enviados por el Z80, o bien enviarlos a solicitud del mismo. Es decir, el microcontrolador trabaja en modo esclavo. De esta manera es posible establecer intercambio de datos controlados desde el Z80. 



Otras posibles habilidades del cartucho

Cloud disk
Para esto es necesario implementar una unidad de disco que permita acceder a un repositorio en la nube. Sería ideal si se pudiese conectar a por ejemplo google drive. 
Mapear una unidad de red a una carpeta compartida de windows o linux.
Audio
El ESP32 tiene la posibilidad de generar audio a través de dos salidas que poseen un DAC. Sabiendo esto, el cartucho tiene previsto evolucionar sobre este punto de manera de conectar estas salidas a la entrada de audio de MSX. En teoría sería posible generar sonidos tan complejos como la programación del ESP32 lo permita.
RS232 TTL
Se ha dejado disponible una UART en el cartucho de manera que el cartucho sirva como una interfaz serie. 
Sería necesario implementar las extensiones BASIC correspondientes
CALL COMBREAK
CALL COMDTR
CALL COM GOSUB
CALL COMHELP
CALL COMINI
CALL COMOFF
CALL COMON
CALL COMSTAT
CALL COMSTOP
CALL COMTERM


© Carlos Escobar 2019
