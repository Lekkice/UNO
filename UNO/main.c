#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>

typedef struct {
    int color;     // 0 = rojo, 1 = amarillo, 2 = celeste, 3 = verde
    int num;       // 1 al 9
    int jugador;   // 1 al 4
    int especial;  // -1 = normal, 0 = cambia color, 1 = +4, 2 = saltar turno, 3 = +2, 4 = cambiar orden
}Carta;


void dibujarCarta(ALLEGRO_BITMAP* cartas, Carta carta, int x, int y)
{
    int anchoCarta = 94;
    int largoCarta = 141;

    if (carta.especial == -1)
    {
        al_draw_bitmap_region(cartas, 0.2 + anchoCarta * (carta.num-1), 1 + largoCarta * carta.color, anchoCarta, largoCarta,
                x - (anchoCarta / 2), y - (largoCarta / 2), 0);
        return;
    }
}

int main()
{
    al_init();
    al_init_image_addon();
    al_install_keyboard();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(1280, 720);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    bool redraw = true;
    bool done = false;
    ALLEGRO_EVENT event;

    ALLEGRO_BITMAP* cartas = al_load_bitmap("cartas.png");

    al_start_timer(timer);
    while (1)
    {
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
        else if ((event.type == ALLEGRO_EVENT_KEY_DOWN) || (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE))
            break;

        switch (event.type)
        {
            case ALLEGRO_EVENT_TIMER:
                redraw = true;
                break;

            case ALLEGRO_EVENT_KEY_DOWN:
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                done = true;
                break;
        }

        if (done) break;

        if (redraw && al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(255, 255, 255));
            //al_draw_text(font, al_map_rgb(0, 0, 0), 250, 200, 0, "Hola");

            Carta carta;
            carta.color = 1; carta.num = 6; carta.especial = -1;

            dibujarCarta(cartas, carta, 1280 / 2, 720 / 2);
            al_flip_display();

            redraw = false;
        }
    }

    al_destroy_bitmap(cartas);

    al_destroy_font(font);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}