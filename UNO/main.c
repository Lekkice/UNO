#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
}Carta;

typedef struct {
    List* listaCartas;
    int cantidad;
    int jugador; // 1 al 4
    bool esBot;
}Jugador;

typedef struct {
    List* jugadores;
    List* cartasJugadas;
    List* mazo;
    int turnoJugador;
    int pausa;
}Estado;

void menuEmpezarJuego(ALLEGRO_TIMER*, ALLEGRO_EVENT_QUEUE*);

void eliminarBotones(List* botones) {
    Boton* boton = popCurrent(botones);
    while (boton) {
        al_destroy_bitmap(boton->imagen);
        free(boton);

        boton = popCurrent(botones);
    }
}

Boton* crearBoton(ALLEGRO_BITMAP* imagen, int ancho, int largo, int posX, int posY, int id) {
    Boton* boton = (Boton*)malloc(sizeof(Boton));
    boton->imagen = imagen;
    boton->ancho = ancho;
    boton->largo = largo;
    boton->posX = posX;
    boton->posY = posY;
    boton->id = id;
    return boton;
}

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

void dibujarBotones(List* botones)
{
    Boton* boton = firstList(botones);
    while (boton) {
        dibujarBoton(boton);
        boton = nextList(botones);
    }
}

void generarMazo(Carta* arregloCartas[], List* mazo) {
    int i = 0;
    int j = 0;
    srand(time(0));
    while (i < 104) {
        j = rand() % 50;
        if (arregloCartas[j]->cont != 0) {
            arregloCartas[j]->cont--;
            pushBack(mazo, arregloCartas[j]);
        }
        i = countList(mazo);
    }
}

Carta* crearCarta(int color, int num, int especial)
{
    Carta* carta = (Carta*)malloc(sizeof(Carta));
    carta->color = color, carta->especial = especial, carta->num = num;
    return carta;
}

void sacarCarta(List* mazo, List* listaCartas) {
    Carta* carta = popFront(mazo);
    pushBack(listaCartas, carta);
}

void dibujarCarta(ALLEGRO_BITMAP* bitCartas, Carta carta, int x, int y)
{
    int anchoCarta = 94;
    int largoCarta = 141;
    switch (carta.especial)
    {
    case -1:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * (carta.num - 1), 1 + largoCarta * carta.color, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 0:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 9, 1 + largoCarta * 0, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 1:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * 9, 1 + largoCarta * 2, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 2:
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * carta.color, 1 + largoCarta * 4, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 3:
        al_draw_bitmap_region(bitCartas, 0.2 + (anchoCarta * 4) + anchoCarta * carta.color, 1 + largoCarta * 4, anchoCarta, largoCarta,
            x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        break;

    case 4:
        if (carta.color <= 1) {
            al_draw_bitmap_region(bitCartas, 0.2 + (anchoCarta * 7) + anchoCarta * carta.color, 1 + largoCarta * 4, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        else {
            al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * (carta.color - 2), 1 + largoCarta * 5, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        }
        break;
    }
}

void dibujarCartas(Jugador* jugador, ALLEGRO_BITMAP* bitCartas)
{
    List* lista = jugador->listaCartas;
    Carta* carta = firstList(lista);
    int x = 100, y = 600;
    while (carta)
    {
        dibujarCarta(bitCartas, *carta, x, y);
        x += 100;
        carta = nextList(jugador->listaCartas);
    }
}

int encontrarCarta(int mx, int my)
{
    int i;
    for (i = 1; i <= 16; i++) // 100, 600; 94,141
    {
        if (((my > 530) && (my < 670)))
            if ((mx > (100 * i - 94 / 2)) && (mx < (100 * i + 94 / 2)))
                return i;
    }
    return -1;
}

/*bool sePuedeJugar(Estado* estado, Jugador* jugador, int cartaMouse) {
    List* lista = jugador->listaCartas;
    Carta* carta = firstList(lista);

    for (int i = 0; i < cartaMouse - 1; i++) {
        carta = nextList(lista);
    }

}*/

void jugarCarta(Estado* estado, Jugador* jugador, int cartaMouse)
{
    List* lista = jugador->listaCartas;
    Carta* carta = firstList(lista);
    for (int i = 0; i < cartaMouse - 1; i++)
    {
        carta = nextList(lista);
    }
    pushFront(estado->cartasJugadas, carta);
    popCurrent(lista);
}

void terminarTurno(Estado* estado)
{
    int numJugadores = countList(estado->jugadores);
    if (estado->turnoJugador == numJugadores)
    {
        estado->turnoJugador = 0;
        return;
    }
    estado->turnoJugador++;
    estado->pausa = 1;
}

void menuCrearPartida(ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue) {
    List* botones = createList();
    int mx = 0, my = 0, click = 0, botonMouse, numPlayers, dif;
    bool redraw;
    bool done = false;
    bool flag;
    ALLEGRO_EVENT event;
    ALLEGRO_BITMAP* fondo = al_load_bitmap("fondo.png");

    ALLEGRO_BITMAP* botonPrueba = al_load_bitmap("Next.png");
    Boton *boton = crearBoton(botonPrueba, 513, 181, 120, 120, 0);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("Exit.png");
    boton = crearBoton(botonPrueba, 522, 183, 120, 120, 1);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("Next.png");
    boton = crearBoton(botonPrueba, 513, 181, 120, 180, 2);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("Exit.png");
    boton = crearBoton(botonPrueba, 522, 183, 120, 180, 3);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("play.png");
    boton = crearBoton(botonPrueba, 510, 372, 100, 250, 4);
    pushFront(botones, boton);

    botonPrueba = al_load_bitmap("close.png");
    boton = crearBoton(botonPrueba, 517, 240, 200, 250, 5);
    pushFront(botones, boton);

    while (1)
    {
        // break; // eliminar cuando el menú esté listo

        int botonMouse = -1;
        int click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            printf("x = %i, y = %i\n", mx, my);
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (done) break;

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);

            switch (botonMouse) {
            case 0:
                //numPlayers++;
            case 1:
                //numPlayer--;
            case 2:
                dif++;
            case 3:
                dif--;
            case 4:
                printf("se dio a play\n");
                break;
            case 5:
                printf("se dio a salir");
                flag = false;
                break;
            }
        }

        if (redraw && al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(fondo, 0, 0, 0);

            dibujarBotones(botones);

            al_flip_display();

            redraw = false;
        }

        //printf("%i , %i", numPlayers, dif);
    }
    
    if (flag)menuEmpezarJuego(timer, queue);
}

int main()
{
    al_init();
    al_init_image_addon();
    al_install_mouse();
    al_init_primitives_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(1280, 720);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    bool redraw = true;
    bool done = false;
    bool flag;
    ALLEGRO_EVENT event;

    al_start_timer(timer);
    ALLEGRO_BITMAP* fondo = al_load_bitmap("fondo.png");

    List* botones = createList(); // lista con botones del menú principal

    ALLEGRO_BITMAP* botonPrueba = al_load_bitmap("button.png");
    Boton* boton = crearBoton(botonPrueba, 513, 181, 500, 500, 0);
    pushFront(botones, boton);

    //menuEmpezarJuego(timer, queue); // se debe llamar al presionar un botón en el menú principal

    int botonMouse, click, mx, my;
    while (1)
    {
        break; // eliminar cuando el menú esté listo

        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            printf("x = %i, y = %i\n", mx, my);
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (done) break;

        if (click && al_is_event_queue_empty(queue))
        {
            botonMouse = encontrarBoton(botones, mx, my);

            if(botonMouse == 0)flag = menuCrearPartida(timer, queue);
            //código que maneja los casos usando el id de los botones
        }

        if (redraw && al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(fondo, 0, 0, 0);

            dibujarBotones(botones);

            al_flip_display();

            redraw = false;
        }

    }

    al_destroy_font(font);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}

void menuEmpezarJuego(ALLEGRO_TIMER* timer, ALLEGRO_EVENT_QUEUE* queue) {
    int i,j;
    int posArr;
    bool redraw = true;
    bool done = false;
    ALLEGRO_EVENT event;

    ALLEGRO_BITMAP* fondo = al_load_bitmap("fondo.png");
    ALLEGRO_BITMAP* bitCartas = al_load_bitmap("cartas.png");

    Jugador* jugador;
    jugador = (Jugador*)malloc(sizeof(Jugador));
    jugador->listaCartas = createList();

    Estado* estado = (Estado*)malloc(sizeof(Estado));
    estado->jugadores = createList();
    estado->cartasJugadas = createList();
    estado->turnoJugador = 0;
    estado->mazo = createList(); // generarMazo()
    estado->pausa = 0;

    int mx = 0, my = 0, click = 0, cartaMouse, botonMouse;
    Carta* cartaJugada = NULL;

    Carta* carta;        //crear arreglo con todas las cartas
    posArr = 0;
    Carta* arregloCartas[50];
    for (i = 0; i < 4; i++) {
        for (j = 1; j < 10; j++) {
            carta = (Carta*)malloc(sizeof(Carta));
            carta->color = i;
            carta->num = j;
            if (carta->num == 0) {
                carta->cont = 1;
            }
            else {
                carta->cont = 2;
            }
            carta->especial = -1;
            arregloCartas[posArr] = carta;
            posArr++;
            //printf(" %d %d %d %d \n", carta->num, carta->color, carta->especial,carta->cont);
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 2; j < 5; j++) {
            carta = (Carta*)malloc(sizeof(Carta));
            carta->num = NULL;
            carta->color = i;
            carta->especial = j;
            carta->cont = 2;
            arregloCartas[posArr] = carta;
            posArr++;
            //printf(" %d %d %d %d \n", carta->num, carta->color, carta->especial,carta->cont);
        }
    }
    for (i = 0; i < 2; i++) {
        carta = (Carta*)malloc(sizeof(Carta));
        carta->num = NULL;
        carta->color = NULL;
        carta->especial = i;
        carta->cont = 4;
        arregloCartas[posArr] = carta;
        posArr++;
        //printf(" %d %d %d %d\n", carta->num, carta->color, carta->especial,carta->cont);
    }

    generarMazo(arregloCartas,estado->mazo);         //funcion para crear el mazo (hay que revisar si funciona) .si funciona.

    /*jugador = firstList(estado->jugadores);
    while (jugador) {                              //darle a cada jugador sus 7 cartas iniciales
        while (countList(jugador->listaCartas) < 7) {
            sacarCarta(estado->mazo, jugador->listaCartas);
        }
        jugador = nextList(estado->jugadores);
    }*/
    
    while (countList(jugador->listaCartas) < 7) {
        sacarCarta(estado->mazo, jugador->listaCartas);
    }
    
    //pushBack(jugador->listaCartas, crearCarta(0, 1, -1));
    //pushBack(jugador->listaCartas, crearCarta(1, 1, -1));

    List* botones = createList();
    pushBack(botones, crearBoton(al_load_bitmap("backside.png"), 94, 141, (1280 / 2) + 100, 720/2, 1));
    // dibujarCarta(bitCartas, *cartaJugada, (1280 / 2) - 200, 720 / 2);

    while (1)
    {
        cartaMouse = -1;
        botonMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            //printf("x = %i, y = %i\n", mx, my);
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            click = 1;
            break;
        }

        if (done) break;

        if (click && al_is_event_queue_empty(queue))
        {
            //revisa si el mouse está sobre un botón o carta
            if (!estado->pausa)
            {
                cartaMouse = encontrarCarta(mx, my);
                if (cartaMouse != -1 && cartaMouse <= countList(jugador->listaCartas))
                {
                    jugarCarta(estado, jugador, cartaMouse);
                    terminarTurno(estado);
                }

                botonMouse = encontrarBoton(botones, mx, my);
                if (botonMouse != -1)
                {
                    switch (botonMouse)
                    {
                    case 1:
                        printf("sacar carta\n");
                        sacarCarta(estado->mazo, jugador->listaCartas);
                        break;
                    }
                }
            }
            else
            {
                estado->pausa = 0;
            }
        }

        if (redraw && al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));


            al_draw_bitmap(fondo, 0, 0, 0);


            dibujarBotones(botones);

            if (!estado->pausa) dibujarCartas(jugador, bitCartas);

            cartaJugada = firstList(estado->cartasJugadas);
            if (cartaJugada)
            {
                dibujarCarta(bitCartas, *cartaJugada, (1280 / 2) - 200, 720 / 2);
            }

            al_flip_display();

            redraw = false;
        }
    }

    if (done)
    {
        al_destroy_bitmap(fondo);
        al_destroy_bitmap(bitCartas);
        return;
    }
}