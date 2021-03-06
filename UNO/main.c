#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include "list.h"
#include "treemap.h"

typedef struct {
    ALLEGRO_BITMAP* imagen;
    int ancho; int largo;
    int posX; int posY;
    int id; // se usa para decidir cual función se debe llamar
}Boton;

typedef struct {
    int color;     // 0 = rojo, 1 = amarillo, 2 = celeste, 3 = verde
    int num;       // 1 al 9
    int especial;  // -1 = normal, 0 = cambia color, 1 = +4, 2 = saltar turno, 3 = +2, 4 = cambiar orden
    int cont;
    int valorJugada;
    bool sePuede;
}Carta;

typedef struct {
    List* listaCartas;
    int cantidad;
    int num; // 0 al num máx de jugadores
    bool esBot;
    int* points;  //puntuacion
}Jugador;

typedef struct {
    Jugador** jugadores;
    int jugadorActual;
    int numJugadores;
    List* cartasJugadas;
    List* mazo;
    TreeMap* puntuacion;
    int direccion;
    float volumen;
}Estado;

void menuPuntuacion(ALLEGRO_EVENT_QUEUE*, Estado*);
void menuEmpezarJuego(ALLEGRO_EVENT_QUEUE*, int, ALLEGRO_FONT*, Estado*);
bool sePuedeJugar(Carta*, Carta*);

// genera un mazo utilizando un arreglo con todas las cartas que se recorre de forma aleatoria
void generarMazo(List* mazo) {
    int i = 0;
    int j = 0;
    int posArr = 0;
    Carta arregloCartas[50];

    for (i = 0; i < 4; i++) {
        for (j = 1; j < 10; j++) {
            Carta carta;
            carta.color = i;
            carta.num = j;
            if (carta.num == 0) {
                carta.cont = 1;
            }
            else {
                carta.cont = 2;
            }
            carta.especial = -1;
            arregloCartas[posArr] = carta;
            posArr++;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 2; j < 5; j++) {
            Carta carta;
            carta.num = 0;
            carta.color = i;
            carta.especial = j;
            carta.cont = 2;
            arregloCartas[posArr] = carta;
            posArr++;
        }
    }
    for (i = 0; i < 2; i++) {
        Carta carta;
        carta.num = 0;
        carta.color = -1;
        carta.especial = i;
        carta.cont = 4;
        arregloCartas[posArr] = carta;
        posArr++;
    }

    srand(time(0));
    while (i < 104) {
        j = rand() % 50;
        if (arregloCartas[j].cont != 0) {
            arregloCartas[j].cont--;
            Carta* carta = (Carta*)malloc(sizeof(Carta));
            if (carta == NULL) {
                printf("error de malloc carta\n");
                exit(1);
            }
            *carta = arregloCartas[j];
            pushBack(mazo, carta);
        }
        i = countList(mazo);
    }
}

// inicializa y retorna un jugador
Jugador* crearJugador(int num, bool esBot) {
    Jugador* jugador = (Jugador*)malloc(sizeof(Jugador));
    if (jugador == NULL) {
        printf("error de malloc jugador\n");
        exit(1);
    }
    int* points = (int*)malloc(sizeof(int));
    if (points == NULL) {
        printf("error de malloc points\n");
        exit(1);
    }
    *points = 0;
    jugador->cantidad = 7;
    jugador->listaCartas = createList();
    jugador->num = num;
    jugador->esBot = esBot;
    jugador->points = points;
    return jugador;
}

// inicializa y retorna un botón
Boton* crearBoton(ALLEGRO_BITMAP* imagen, int ancho, int largo, int posX, int posY, int id) {
    Boton* boton = (Boton*)malloc(sizeof(Boton));
    if (boton == NULL) {
        printf("error de malloc botón\n");
        exit(1);
    }
    boton->imagen = imagen;
    boton->ancho = ancho;
    boton->largo = largo;
    boton->posX = posX;
    boton->posY = posY;
    boton->id = id;
    return boton;
}

// elimina los botones en la lista y libera la memoria
void eliminarBotones(List* botones) {
    Boton* boton = popCurrent(botones);
    while (boton) {
        al_destroy_bitmap(boton->imagen);
        free(boton);

        boton = popCurrent(botones);
    }
}

// función de comparación para el mapa
int lower_than(void* key1, void* key2)
{
    int a = *(int*)key1;
    int b = *(int*)key2;
    if (a < b) return 1;
    return 0;
}

// termina el turno del jugador actual y actualiza el estado
void terminarTurno(Estado* estado)
{
    estado->jugadorActual += estado->direccion;
    if (estado->jugadorActual >= estado->numJugadores) estado->jugadorActual = 0;
    if (estado->jugadorActual < 0) estado->jugadorActual = estado->numJugadores - 1;
    printf("turno de jugador %i\n", estado->jugadorActual);
}

// saca la última carta contenida en el mazo
void sacarCarta(Estado* estado, Jugador* jugador, ALLEGRO_SAMPLE* sonido) {
    Carta* carta = popFront(estado->mazo);
    pushBack(jugador->listaCartas, carta);
    jugador->cantidad++;
    al_play_sample(sonido, estado->volumen, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    printf("saca carta\n");
}

void calcularPuntuacion(Estado* estado) {
    for (int i = 0; i < estado->numJugadores; i++) {
        Jugador* jugador = estado->jugadores[i];
        Carta* carta = firstList(jugador->listaCartas);
        int cont = 0;

        while (carta) {
            if (carta->especial == -1) {
                cont += carta->num;
            }
            if (carta->especial == 0 || carta->especial == 1) {
                cont += 50;
            }
            else cont += 20;
            carta = nextList(jugador->listaCartas);
        }

        *(jugador->points) = cont;

        void* key = jugador->points;
        void* value = jugador;
        insertTreeMap(estado->puntuacion, key, value);
        cont = 0;
    }
}

bool jugarCarta(Estado* estado, Jugador* jugador, int posCarta, ALLEGRO_EVENT_QUEUE* queue, ALLEGRO_SAMPLE* sonidoSacarCarta, ALLEGRO_FONT* font)
{
    List* lista = jugador->listaCartas;
    Carta* carta = firstList(lista);
    // encuentra una carta usando su posición
    for (int i = 0; i < posCarta - 1; i++)
    {
        carta = nextList(lista);
    }
    printf("carta jugada: color = %i, num = %i\n", carta->color, carta->num);

    Carta* cartaJugada = firstList(estado->cartasJugadas);
    if (sePuedeJugar(cartaJugada, carta)) {
        if (!jugador->esBot) {
            if ((carta->especial == 0) || (carta->especial == 1)) carta->color = asignarColor(queue);
        }
        else {
            if ((carta->especial == 0) || (carta->especial == 1)) carta->color = elegirMejorColor(jugador->listaCartas);
        }

        pushFront(estado->cartasJugadas, carta);
        popCurrent(lista);
        jugador->cantidad--;
    }
    else {
        printf("no se pudo jugar carta, num = %i  color = %i\n", cartaJugada->num, cartaJugada->color);
        return false;
    }

    // si no quedan cartas en el mazo, se genera uno nuevo
    if (countList(estado->mazo) == 0) {
        generarMazo(estado->mazo);
    }

    // si el jugador no tiene cartas, se termina el juego y muestra la puntuación
    if (((countList(jugador->listaCartas)) == 0)) {
        calcularPuntuacion(estado);
        menuPuntuacion(queue, estado);
    }

    // cartas especiales

    if (carta->especial == 4) {
        if (estado->numJugadores == 2) {
            terminarTurno(estado);
        }
        estado->direccion = estado->direccion * -1;
    }

    if (carta->especial == 2) {
        terminarTurno(estado);
    }

    if ((carta->especial == 1) || (carta->especial == 3)) {
        int i = 0;
        if (carta->especial == 1) {
            i = 4;
        }
        else {
            i = 2;
        }

        terminarTurno(estado);

        jugador = estado->jugadores[estado->jugadorActual];

        for (int j = 0; j < i; j++) {
            sacarCarta(estado, jugador, sonidoSacarCarta);
        }
        terminarTurno(estado);

        return true;
    }
    terminarTurno(estado);
    return true;
}

// revisa si una carta se puede jugar y retorna un bool
bool sePuedeJugar(Carta* cartaJugada, Carta* carta) {
    if ((carta->especial == 0) || (carta->especial == 1)) return true;

    if (((cartaJugada->especial == 0) || (cartaJugada->especial == 1)) && (cartaJugada->num == -1)) return true;

    if (carta->especial == -1) {
        if ((carta->num == cartaJugada->num) || (carta->color == cartaJugada->color)) return true;
    }

    if (carta->especial > 1) {
        if ((carta->especial == cartaJugada->especial) || (carta->color == cartaJugada->color)) return true;
    }

    return false;
}

// muestra la cantidad de cartas que el resto de los jugadores tiene
void dibujarEstado(Estado* estado, ALLEGRO_FONT* font) {
    int numJugadores = estado->numJugadores;
    Jugador* jugador = NULL;
    for (int i = 0; i < numJugadores; i++) {
        jugador = estado->jugadores[i];
        int x = 1100, y = 100 + (i * 20);
        int count = countList(jugador->listaCartas);
        al_draw_textf(font, al_map_rgb(10, 10, 10), x, y, 0, "Jugador %i    %i cartas", i+1, count);
    }
}

// muestra la puntuación de todos los jugadores
void dibujarPuntuacion(Estado* estado, ALLEGRO_FONT* font) {
    int numJugadores = estado->numJugadores;
    Pair *puntuacion = firstTreeMap(estado->puntuacion);
    for (int i = 0; i < numJugadores; i++) {
        Jugador* jugador = puntuacion->value;
        int x = 250, y = 100 + (i * 70);
        int count = countList(jugador->listaCartas);
        al_draw_textf(font, al_map_rgb(10, 10, 10), x - 50, y, 0, " %i°", i+1);
        al_draw_textf(font, al_map_rgb(10, 10, 10), x + 100, y, 0, "Jugador %i", jugador->num + 1);
        al_draw_textf(font, al_map_rgb(10, 10, 10), x + 400, y, 0, "puntos: %i", *jugador->points);
        puntuacion = nextTreeMap(estado->puntuacion);
    }
}

// función de bot. elige el color con las mayores posibilidades
int elegirMejorColor(List* listaCartas) {
    int max = 0;
    int num = 0;
    int color = 0;
    for (int i = 0; i < 4; i++) {
        Carta cartaAux;
        cartaAux.color = i;
        cartaAux.especial = 2;

        Carta* carta = firstList(listaCartas);
        num = 0;
        while (carta) {
            if (sePuedeJugar(&cartaAux, carta)) num++;
            carta = nextList(listaCartas);
        }
        if (num > max) {
            max = num;
            color = i;
        }
    }
    num = max;
    printf("bot elige color %i\n", color);
    return color;
}

// calcula las cartas que se pueden jugar el siguiente turno asumiendo que el color y el número no cambian
int calcularPosibilidades(List* cartas, Carta* cartaJugada) {
    int num = 0;
    if ((cartaJugada->especial == 0) || (cartaJugada->especial == 1)) {
        int max = 0;
        for (int i = 0; i < 4; i++) {
            Carta cartaAux;
            cartaAux.color = i;
            cartaAux.especial = 2;

            Carta* carta = firstList(cartas);
            while (carta) {
                if (sePuedeJugar(&cartaAux, carta)) num++;
                carta = nextList(cartas);
            }
            if (num > max) max = num;
        }
        num = max;
    }
    else {
        Carta* carta = firstList(cartas);
        while (carta) {
            if (sePuedeJugar(cartaJugada, carta)) num++;
            carta = nextList(cartas);
        }
    }
    return num - 1;
}

// función de bot. encuentra la carta con las mayores posibilidades
int encontrarMejorCarta(Estado* estado, List* listaCartas) {
    Carta* cartaJugada = firstList(listaCartas);
    int maxIdx = -1;
    int max = 0, i = 0;
    while (cartaJugada) {
        int posib;
        if (sePuedeJugar(firstList(estado->cartasJugadas), cartaJugada)) {
            posib = calcularPosibilidades(listaCartas, cartaJugada);
            if (posib > max) {
                max = posib;
                maxIdx = i;
            }
        }
        i++;
        cartaJugada = nextList(listaCartas);
    }
    printf("carta maxIdx %i con %i posibilidades\n", maxIdx, max);
    return maxIdx;
}

// recibe la posición del mouse y una lista de botones, retorna el id de un botón si la posición del mouse está sobre este
int encontrarBoton(List* botones, int mx, int my)
{
    Boton* boton = firstList(botones);
    while (boton) {
        int x = boton->posX; int y = boton->posY;
        int largo = boton->largo; int ancho = boton->ancho;
        if (((my > (y - largo / 2)) && (my < (y + largo / 2))))
            if (((mx > (x - ancho / 2)) && (mx < (x + ancho / 2))))
                return boton->id;
        boton = nextList(botones);
    }
    return -1;
}

void dibujarBoton(Boton* boton)
{
    int ancho = boton->ancho;
    int largo = boton->largo;
    al_draw_bitmap(boton->imagen, boton->posX - (ancho / 2), boton->posY - (largo / 2), 0);
}

// muestra una lista de botones
void dibujarBotones(List* botones)
{
    Boton* boton = firstList(botones);
    while (boton) {
        dibujarBoton(boton);
        boton = nextList(botones);
    }
}

void dibujarCarta(ALLEGRO_BITMAP* bitCartas, Carta* carta, int x, int y, bool esMazo)
{
    int anchoCarta = 94;
    int largoCarta = 141;
    switch (carta->especial)
    {
    case -1:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * (carta->num - 1), 1 + largoCarta * carta->color, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 0:
        if (carta->color == -1) {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 9, 1 + largoCarta * 0, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        else {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 6 + anchoCarta * (carta->color), 1 + largoCarta * 5, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        };
        break;
        
    case 1:
        if (esMazo) {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 9, 1 + largoCarta * 2, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        else {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 2 + anchoCarta * (carta->color), 1 + largoCarta * 5, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        };
        break;

    case 2:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * carta->color, 1 + largoCarta * 4, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 3:
        al_draw_bitmap_region(bitCartas, 0.2 + (anchoCarta * 4) + anchoCarta * carta->color, 1 + largoCarta * 4, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 4:
        if (carta->color <= 1) {
            al_draw_bitmap_region(bitCartas, 0.2 + (anchoCarta * 8) + anchoCarta * carta->color, 1 + largoCarta * 4, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        else {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * (carta->color - 2), 1 + largoCarta * 5, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        break;
    }
}

// muestra las cartas en la mano del jugador
void dibujarCartas(Jugador* jugador, ALLEGRO_BITMAP* bitCartas)
{
    List* lista = jugador->listaCartas;
    Carta* carta = firstList(lista);
    int count = countList(lista);
    int i = 0;
    int x = 100, y = 525;
    while (i < (count - 12)) {
        dibujarCarta(bitCartas, carta, x, y, true);
        x += 100;
        i++;
        carta = nextList(jugador->listaCartas);
    }
    x = 100, y = 600;
    while (carta)
    {
        dibujarCarta(bitCartas, carta, x, y, true);
        x += 100;
        carta = nextList(jugador->listaCartas);
        i++;
    }
}

// recibe la posición del mouse, retorna la posición de la carta si el mouse está sobre una
int encontrarCarta(int mx, int my, int cantidad)
{
    int i;
    int dif = cantidad - 12;
    if (dif < 0) dif = 0;
    for (i = 1; i <= 12; i++)
    {
        if (((my > 530) && (my < 670)))
            if ((mx > (100 * i - 94 / 2)) && (mx < (100 * i + 94 / 2)))
                return dif + i;

        if (((my > 455) && (my < 530)) && dif != 0)
            if ((mx > (100 * i - 94 / 2)) && (mx < (100 * i + 94 / 2)))
                return i;
    }
    return -1;
}

// recibe el color que el jugador quiere usar
int asignarColor(ALLEGRO_EVENT_QUEUE* queue) {
    List* botones = createList();
    int mx = 0, my = 0, click, botonMouse;
    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/fondo.png");

    ALLEGRO_BITMAP* botonRueda = al_load_bitmap("assets/Rojo.png");
    Boton* boton = crearBoton(botonRueda, 102, 98, 300, 250, 0);
    pushFront(botones, boton);

    botonRueda = al_load_bitmap("assets/Azul.png");
    boton = crearBoton(botonRueda, 100, 96, 200, 350, 2);
    pushFront(botones, boton);

    botonRueda = al_load_bitmap("assets/Verde.png");
    boton = crearBoton(botonRueda, 100, 98, 200, 250, 3);
    pushFront(botones, boton);

    botonRueda = al_load_bitmap("assets/Amarillo.png");
    boton = crearBoton(botonRueda, 102, 96, 300, 350, 1);
    pushFront(botones, boton);

    while (1)
    {
        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            exit(0);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);
            if (botonMouse != -1) {
                eliminarBotones(botones);
                return botonMouse;
            }
        }

        if (al_is_event_queue_empty(queue))
        {
            dibujarBotones(botones);

            al_flip_display();
        }
    }
}

void mostrarTutorial(ALLEGRO_EVENT_QUEUE* queue, ALLEGRO_BITMAP* fondo) {
    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* tutorial1 = al_load_bitmap("assets/Tutorial1.png");
    ALLEGRO_BITMAP* tutorial2 = al_load_bitmap("assets/Tutorial2.png");
    int i = 0;
    while (true) {
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) i++;

        al_draw_bitmap(fondo, 0, 0, 0);

        if (i == 0) al_draw_bitmap(tutorial1, 400, 50, 0);
        if (i == 1) al_draw_bitmap(tutorial2, 400, 50, 0);
        al_flip_display();

        if (i == 2) {
            al_destroy_bitmap(tutorial1);
            al_destroy_bitmap(tutorial2);
            break;
        }
    }
}

void menuPuntuacion(ALLEGRO_EVENT_QUEUE* queue, Estado* estado) {
    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/MenuUno.png");
    ALLEGRO_FONT* font = al_load_ttf_font("assets/edo.ttf", 50, 0);

    al_draw_bitmap(fondo, 0, 0, 0);
    dibujarPuntuacion(estado, font);
    al_flip_display();

    while (1) {
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) exit(0);
    }
}

void menuCrearPartida(ALLEGRO_EVENT_QUEUE* queue, Estado* estado) {
    List* botones = createList();
    int mx = 0, my = 0, click = 0, dif = 1;
    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/fondo.png");
    ALLEGRO_FONT* font = al_load_ttf_font("assets/edo.ttf", 15, 0);
    ALLEGRO_FONT* fontMenu = al_load_ttf_font("assets/edo.ttf", 30, 0);

    ALLEGRO_BITMAP* bitLeft = al_load_bitmap("assets/Left.png");
    ALLEGRO_BITMAP* bitRight = al_load_bitmap("assets/Right.png");
    ALLEGRO_BITMAP* bitPlay = al_load_bitmap("assets/play.png");
    ALLEGRO_BITMAP* bitClose = al_load_bitmap("assets/close.png");

    int x = 1280 / 4, y = 720;
    Boton *boton = crearBoton(bitLeft, 50, 50, x, 120 + 190, 0);
    pushFront(botones, boton);
    boton = crearBoton(bitRight, 50, 50, x * 3, 120 + 190, 1);
    pushFront(botones, boton);
    
    boton = crearBoton(bitPlay, 200, 146, 222, 600, 4);
    pushFront(botones, boton);

    boton = crearBoton(bitClose, 107, 49, 1000, 600, 5);
    pushFront(botones, boton);

    int numPlayers = 2;
    while (1)
    {
        int botonMouse = -1;
        int click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            exit(0);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);

            switch (botonMouse) {
            case 0:
                if (numPlayers > 2) numPlayers--;
                printf("%i\n", numPlayers);
                break;
            case 1:
                if (numPlayers < 5) numPlayers++;
                printf("%i\n", numPlayers);
                break;
            case 4:
                al_destroy_bitmap(fondo);
                al_destroy_font(fontMenu);
                eliminarBotones(botones);

                menuEmpezarJuego(queue, numPlayers, font, estado);
                break;
            case 5:
                al_destroy_bitmap(fondo);
                al_destroy_font(fontMenu);
                eliminarBotones(botones);

                exit(0);
            }
        }

        if (al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(fondo, 0, 0, 0);

            al_draw_textf(fontMenu, al_map_rgb(10, 10, 10), 1280 / 2 - 105, 100 + 190, 0, "%i   Jugadores", numPlayers);

            dibujarBotones(botones);

            al_flip_display();

        }

    }
}

void menuConfiguraciones(ALLEGRO_EVENT_QUEUE* queue, Estado* estado, ALLEGRO_AUDIO_STREAM* musica){
    int mx = 0, my = 0, click = 0, botonMouse = -1;
    float volumenMusica = 1.0;

    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/fondo.png");
    ALLEGRO_FONT* font = al_load_ttf_font("assets/edo.ttf", 15, 0);
    ALLEGRO_FONT* fontMenu = al_load_ttf_font("assets/edo.ttf", 30, 0);

    ALLEGRO_BITMAP* bitLeft = al_load_bitmap("assets/Left.png");
    ALLEGRO_BITMAP* bitRight = al_load_bitmap("assets/Right.png");
    ALLEGRO_BITMAP* bitClose = al_load_bitmap("assets/close.png");

    List* botones = createList();
    int x = 1280 / 4, y = 720;

    Boton* boton = crearBoton(bitLeft, 50, 50, x, 120, 0);
    pushFront(botones, boton);
    boton = crearBoton(bitRight, 50, 50, x * 3, 120, 1);
    pushFront(botones, boton);

    boton = crearBoton(bitLeft, 50, 50, x, 320, 2);
    pushFront(botones, boton);
    boton = crearBoton(bitRight, 50, 50, x * 3, 320, 3);
    pushFront(botones, boton);

    boton = crearBoton(bitClose, 107, 49, 222, 600, 4);
    pushFront(botones, boton);
    
    while (1)
    {
        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            exit(0);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);

            switch (botonMouse) {
            case 0:
                if (estado->volumen > 0) estado->volumen -= 0.1;
                if (estado->volumen < 0) estado->volumen = 0.0;
                break;
            case 1:
                if (estado->volumen < 1) estado->volumen += 0.1;
                break;
            case 2:
                if (volumenMusica > 0) volumenMusica -= 0.1;
                if (volumenMusica < 0) volumenMusica = 0.0;
                al_set_audio_stream_gain(musica, volumenMusica);
                break;
            case 3:
                if (volumenMusica < 1) volumenMusica += 0.1;
                al_set_audio_stream_gain(musica, volumenMusica);
                break;
            case 4:
                al_destroy_bitmap(fondo);
                al_destroy_font(fontMenu);
                eliminarBotones(botones);

                return;
            }
        }

        if (al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(fondo, 0, 0, 0);

            dibujarBotones(botones);

            al_draw_textf(fontMenu, al_map_rgb(10, 10, 10), 1280 / 2 - 155, 100, 0, "Volumen de sonido  %.0f", estado->volumen * 10);
            al_draw_textf(fontMenu, al_map_rgb(10, 10, 10), 1280 / 2 - 155, 300, 0, "Volumen de música  %.0f", volumenMusica * 10);

            al_flip_display();
        }

    }
}

void menuEmpezarJuego(ALLEGRO_EVENT_QUEUE* queue, int numPlayers, ALLEGRO_FONT* font, Estado* estado) {
    ALLEGRO_EVENT event;

    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/fondo.png");
    ALLEGRO_BITMAP* bitCartas = al_load_bitmap("assets/cartas.png");
    ALLEGRO_BITMAP* bitmapBS = al_load_bitmap("assets/backside.png");

    ALLEGRO_SAMPLE* sonidoJugarCarta = al_load_sample("assets/jugar carta.wav");
    ALLEGRO_SAMPLE* sonidoSacarCarta = al_load_sample("assets/sacar carta.wav");

    estado->numJugadores = numPlayers;


    Jugador* jugadores[5];
    for (int i = 0; i < estado->numJugadores; i++) {
        bool esBot = true;
        if (i == 0) esBot = false;
        jugadores[i] = crearJugador(i, esBot);
    }

    estado->jugadores = jugadores;

    estado->cartasJugadas = createList();
    estado->mazo = createList();
    estado->puntuacion = createTreeMap(lower_than);
    estado->jugadorActual = 0;
    estado->direccion = 1;

    Carta* cartaJugada = NULL;

    generarMazo(estado->mazo);
    Carta* carta = popCurrent(estado->mazo);
    if ((carta->especial == 0) || (carta->especial == 1)) {
        carta->color = 0;
    }
    pushBack(estado->cartasJugadas, carta);

    for (int i = 0; i < estado->numJugadores; i++) {
        Jugador* jugador = estado->jugadores[i];
        while (countList(jugador->listaCartas) < 7) {
            Carta* carta = popFront(estado->mazo);
            pushBack(jugador->listaCartas, carta);
            jugador->cantidad++;
        }
    }

    List* botones = createList();
    pushBack(botones, crearBoton(bitmapBS, 94, 141, (1280 / 2) + 100, 720 / 2, 1));

    int mx = 0, my = 0, click = 0, cartaMouse = -1, botonMouse = -1;
    while (1)
    {
        Jugador* jugador = estado->jugadores[estado->jugadorActual];
        cartaMouse = -1;
        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            exit(0);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (click && al_is_event_queue_empty(queue))
        {
            if (jugador->esBot) {
                int numCarta = encontrarMejorCarta(estado, jugador->listaCartas);
                if (numCarta == -1) {
                    sacarCarta(estado, jugador, sonidoSacarCarta);
                    terminarTurno(estado);
                    continue;
                }
                jugarCarta(estado, jugador, numCarta + 1, queue, sonidoSacarCarta, font);
                al_play_sample(sonidoJugarCarta, estado->volumen, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                printf("jugo el bot %i con la carta %i\n", jugador->num, numCarta);

                continue;
            }
            else {
                int numCartas = countList(jugador->listaCartas);
                cartaMouse = encontrarCarta(mx, my, numCartas);
                if (cartaMouse != -1 && cartaMouse <= numCartas)
                {
                    if (jugarCarta(estado, jugador, cartaMouse, queue, sonidoSacarCarta, font)) {
                        printf("jugo el jugador %i con la carta %i\n", jugador->num, cartaMouse);
                        al_play_sample(sonidoJugarCarta, estado->volumen, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }

                }
            }
            // encuentra la carta que está bajo el mouse y la juega
            botonMouse = encontrarBoton(botones, mx, my);
            if (botonMouse != -1)
            {
                switch (botonMouse)
                {
                case 1:
                    if (!jugador->esBot) {
                        sacarCarta(estado, jugador, sonidoSacarCarta);
                        terminarTurno(estado);
                    }
                    break;
                }
            }
        }

        if (al_is_event_queue_empty(queue))
        {
            // muestra los componentes del juego en la pantalla
            al_clear_to_color(al_map_rgb(255, 255, 255));


            al_draw_bitmap(fondo, 0, 0, 0);


            dibujarBotones(botones);

            if (!jugador->esBot) dibujarCartas(jugador, bitCartas);

            cartaJugada = firstList(estado->cartasJugadas);
            if (cartaJugada)
            {
                dibujarCarta(bitCartas, cartaJugada, (1280 / 2) - 200, 720 / 2, false);
            }

            dibujarEstado(estado, font);
            al_flip_display();
        }
    }
}

// menú principal e inicialización de Allegro
int main()
{
    al_init();
    al_init_image_addon();
    al_install_mouse();
    al_init_font_addon();
    al_init_ttf_addon();
    
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(4);

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(1280, 720);

    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_mouse_event_source());

    ALLEGRO_EVENT event;

    ALLEGRO_BITMAP* fondo = al_load_bitmap("assets/fondo.png");

    ALLEGRO_AUDIO_STREAM* musica = al_load_audio_stream("assets/musica.opus", 2, 2048);
    al_set_audio_stream_playmode(musica, ALLEGRO_PLAYMODE_LOOP);
    al_attach_audio_stream_to_mixer(musica, al_get_default_mixer());

    List* botones = createList(); // lista con botones del menú principal
    Estado* estado = (Estado*)malloc(sizeof(Estado));
    if (estado == NULL) {
        printf("error de malloc estado\n");
        exit(1);
    }
    estado->volumen = 1.0;

    ALLEGRO_BITMAP* botonPrueba = al_load_bitmap("assets/play.png");
    Boton *boton = crearBoton(botonPrueba, 200, 146, 625, 150, 0);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("assets/Tutorial.png");
    boton = crearBoton(botonPrueba, 141, 51, 625, 290, 1);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("assets/Config.png");
    boton = crearBoton(botonPrueba, 138, 50, 625, 402, 2);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("assets/Exit.png");
    boton = crearBoton(botonPrueba, 147, 50, 625, 562, 3);
    pushFront(botones, boton);

    int botonMouse = -1, click = 0, mx = 0, my = 0;
    while (1)
    {
        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            exit(0);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);

            switch (botonMouse) {
            case 0:
                eliminarBotones(botones);
                al_destroy_bitmap(fondo);
                menuCrearPartida(queue, estado);
                break;
            case 1:
                mostrarTutorial(queue, fondo);
                break;
            case 2:
                menuConfiguraciones(queue, estado, musica);
                break;
            case 3:
                eliminarBotones(botones);
                al_destroy_bitmap(fondo);
                exit(1);
            }
        }

        if (al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(fondo, 0, 0, 0);

            dibujarBotones(botones);

            al_flip_display();
        }

    }

    al_destroy_display(disp);
    al_destroy_event_queue(queue);

    return 0;
}