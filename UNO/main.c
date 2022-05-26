#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include "list.h"
#include <stdio.h>

typedef struct {
    int color;     // 0 = rojo, 1 = amarillo, 2 = celeste, 3 = verde
    int num;       // 1 al 9
    int especial;  // -1 = normal, 0 = cambia color, 1 = +4, 2 = saltar turno, 3 = +2, 4 = cambiar orden
}Carta;

typedef struct {
    List* listaCartas;
    int cantidad;
    int jugador; // 1 al 4
}Jugador;

typedef struct {
    List* jugadores;
    List* cartasJugadas;
    List* mazo;
    int turnoJugador;
    int pausa;
}Estado;

void dibujarCarta(ALLEGRO_BITMAP* bitCartas, Carta carta, int x, int y)
{
    int anchoCarta = 94;
    int largoCarta = 141;

    if (carta.especial == -1)
    {
        al_draw_bitmap_region(bitCartas, 0.2 + anchoCarta * (carta.num-1), 1 + largoCarta * carta.color, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
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

int esCarta(ALLEGRO_BITMAP* objeto)
{
    if (al_get_bitmap_width(objeto) == 94 && al_get_bitmap_height(objeto) == 141) return 1;
    return 0;
}

int encontrarCarta(Jugador* jugador, int mx, int my)
{
    int i;
    for (i = 1; i <= 16; i++) // 100, 600; 94,141
    {
        if (((my > 530) && (my < 670)))
            if ((mx > (100 * i - 94 / 2)) && (mx < (100 * i + 94 / 2)))
                if (countList(jugador->listaCartas) >= i)
                    return i;
    }
    return -1;
}

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

//void siguienteTurno(Estado* estado)
//{
//    estado->pausa = 0;
//}

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
    ALLEGRO_EVENT event;

    ALLEGRO_BITMAP* fondo = al_load_bitmap("fondo.png");
    ALLEGRO_BITMAP* bitCartas = al_load_bitmap("cartas.png");

    Jugador* jugador;
    jugador = (Jugador*)malloc(sizeof(Jugador));
    jugador->listaCartas = createList();

    Carta* carta = (Carta*)malloc(sizeof(Carta));
    carta->color = 1, carta->especial = -1, carta->num = 3;
    pushBack(jugador->listaCartas, carta);

    carta = (Carta*)malloc(sizeof(Carta));
    carta->color = 0, carta->especial = -1, carta->num = 5;
    pushBack(jugador->listaCartas, carta);

    carta = (Carta*)malloc(sizeof(Carta));
    carta->color = 1, carta->especial = -1, carta->num = 1;
    pushBack(jugador->listaCartas, carta);

    carta = (Carta*)malloc(sizeof(Carta));
    carta->color = 3, carta->especial = -1, carta->num = 2;
    pushBack(jugador->listaCartas, carta);

    carta = (Carta*)malloc(sizeof(Carta));
    carta->color = 2, carta->especial = -1, carta->num = 9;
    pushBack(jugador->listaCartas, carta);

    Estado* estado = malloc((Estado*)sizeof(Estado));
    estado->jugadores = createList();
    estado->cartasJugadas = createList();
    estado->turnoJugador = 0;
    estado->mazo = createList(); // generarMazo()
    estado->pausa = 0;

    int mx = 0, my = 0, click = 0, cartaMouse;
    Carta* cartaJugada;
    al_start_timer(timer);
    while (1)
    {
        cartaMouse = -1;
        click = 0;

        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;

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
            //revisar si el mouse está sobre un botón o carta
            if (!estado->pausa)
            {
                cartaMouse = encontrarCarta(jugador, mx, my);
                if (cartaMouse != -1)
                {
                    printf("%i", cartaMouse);
                    jugarCarta(estado, jugador, cartaMouse);
                    terminarTurno(estado);
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

    al_destroy_bitmap(fondo);
    al_destroy_bitmap(bitCartas);
    al_destroy_font(font);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}