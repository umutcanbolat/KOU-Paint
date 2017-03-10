#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_image.h>

#define WIDTH 1000                                          //!< Width of the screen
#define HEIGHT 600                                          //!< Height of the screen
#define TOOL_SIZE 35                                        //!< Size of the color boxes
#define TOOL_SIZE_2 TOOL_SIZE*1.5                           //!< Size of the tools on toolbox
#define MARGIN 2                                            //!< Space between colors which are on the color palette
#define MARGIN_2 ((2*TOOL_SIZE+3*MARGIN)-TOOL_SIZE*1.5)/2   //!< Space between tools
#define PHI 1.618                                           //!< Golden ratio, is used to increase or decrease thickness value
void log_printf(char const *format, ...);

ALLEGRO_COLOR color;                    //!< drawing color
ALLEGRO_MOUSE_STATE mouse_state;        //!< state of the mouse, clicked?, if so where, etc, etc.
ALLEGRO_KEYBOARD_STATE keyboard_state;  //!< state of the keyboard
ALLEGRO_DISPLAY *display = NULL;        //!< main display
ALLEGRO_DISPLAY *fileio = NULL;         //!< save dialog display
ALLEGRO_EVENT_QUEUE *queue;             //!< event handler queue
ALLEGRO_EVENT event;                    //!< event struct
ALLEGRO_TEXTLOG *textlog = NULL;        //!< terminal output
ALLEGRO_FILECHOOSER *savefile;          //!< save file dialog
int mode = 0;                           //!< Drawing mode; 0:pencil, 1:triangle, 2:spray, 3:line, 4:rectangle, 5:circle
float thickness = 6;                    //!< Thickness of pencil and line tools
bool mouse_over= true;                  //!< Switchs on and off mouseover feature for toolbox



/********************************************//**
 * \brief This is a linked list for revision control
 ***********************************************/
struct Revision {
    ALLEGRO_BITMAP *buffer; //!< Bitmap clone of some backbuffer
    struct Revision *next; //!< Next revision
    struct Revision *prev; //!< Previous revision
};

struct Revision *current; //!< This is the last revision

void add_revision();
void ciz_toolbox();
void pencil();
void triangle();
void line();
void rectangle();
void spray();
void circle();
void save_bitmap();
void open_bitmap();
void undo();
void redo();

int main() {
    /*
        COLORS WHICH ARE ON THE PALETTE
        ALLEGRO_COLOR red       = al_map_rgb(255,0,0);
        ALLEGRO_COLOR orange    = al_map_rgb(255,128,0);
        ALLEGRO_COLOR yellow    = al_map_rgb(255,255,0);
        ALLEGRO_COLOR green1    = al_map_rgb(128,255,0);
        ALLEGRO_COLOR green2    = al_map_rgb(0,255,0);
        ALLEGRO_COLOR turquoise = al_map_rgb(0,255,128);
        ALLEGRO_COLOR gray1     = al_map_rgb(64,64,64);
        ALLEGRO_COLOR black     = al_map_rgb(0,0,0);
        ALLEGRO_COLOR cyan      = al_map_rgb(0,255,255);
        ALLEGRO_COLOR blue      = al_map_rgb(0,128,255);
        ALLEGRO_COLOR blue2     = al_map_rgb(0,0,255);
        ALLEGRO_COLOR purple    = al_map_rgb(128,0,255);
        ALLEGRO_COLOR pink1     = al_map_rgb(255,0,255);
        ALLEGRO_COLOR pink2     = al_map_rgb(255,0,128);
        ALLEGRO_COLOR gray2     = al_map_rgb(128,128,128);
        ALLEGRO_COLOR white     = al_map_rgb(255,255,255);
    */

    srand(time(NULL));  // fix randomness

    // initialize everything
    if(!al_init()) {
        fprintf(stderr, "failed to initialize allegro!\n");
        return -1;
    }
    if(!al_init_native_dialog_addon()) {
        fprintf(stderr, "failed to initialize allegro native!\n");
        return -1;
    }
    if(!al_install_mouse()) {
        fprintf(stderr, "failed to initialize mouse!\n");
        return -1;
    }
    if(!al_install_keyboard()) {
        fprintf(stderr, "failed to initialize keyboard!\n");
        return -1;
    }
    if(!al_init_primitives_addon()) {
        fprintf(stderr, "failed to initialize primitives!\n");
        return -1;
    }
    if(!al_init_image_addon()) {
        fprintf(stderr, "failed to initialize image addon!\n");
        return -1;
    }

    ALLEGRO_COLOR white     = al_map_rgb(255,255,255);

    // create a new WIDTHxHEIGHTpx display, and set title
    display = al_create_display(WIDTH, HEIGHT + 2 * (TOOL_SIZE) + 3 * MARGIN);

    al_set_window_title(display, "KOU Paint");

    ALLEGRO_BITMAP *logo;
    logo = al_load_bitmap("images/koulogo.png");
    al_set_display_icon(display,logo);
    al_destroy_bitmap(logo);
    if(!display) {
        fprintf(stderr, "failed to create display!\n");
        return -1;
    }

    current = (struct Revision*) malloc(sizeof(struct Revision));
    current->buffer = al_clone_bitmap(al_get_backbuffer(display));
    current->prev = NULL;
    current->next = NULL;

    ALLEGRO_COLOR colors[2][8] = {{al_map_rgb(255,0,0),   al_map_rgb(255,128,0), al_map_rgb(255,255,0), al_map_rgb(128,255,0), al_map_rgb(0,255,0),   al_map_rgb(0,255,128), al_map_rgb(64,64,64),    al_map_rgb(0,0,0)},
        {al_map_rgb(0,255,255), al_map_rgb(0,128,255), al_map_rgb(0,0,255),   al_map_rgb(128,0,255), al_map_rgb(255,0,255), al_map_rgb(255,0,128), al_map_rgb(128,128,128), al_map_rgb(255,255,255)}
    };
    color = al_map_rgb(0,0,0);

    // clear screen and draw toolbox
    al_draw_filled_rectangle(0, 0, WIDTH, HEIGHT, white);
    ciz_toolbox();

    // initialize current revision, this will be the first revision
    current = (struct Revision*) malloc(sizeof(struct Revision));
    current->buffer = al_clone_bitmap(al_get_backbuffer(display));
    current->prev = NULL;
    current->next = NULL;

    // handle events from now on
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));

    while(1) {
        al_wait_for_event(queue, &event);   // wait for mouse events
        // this runs when theres an event

        switch(event.type) {
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            // when any mouse button is pressed down, any..
            al_get_mouse_state(&mouse_state); // get mouse state
            if(al_mouse_button_down(&mouse_state, 2)) {
                // al_mouse_button_down takes two parameters, first is state, second is the button we want to check
                // here, 2 means right click, if right click is pressed then clear the screen
                undo();
            } else if(mouse_state.y + 1 >= HEIGHT ) {

                if(mouse_state.x + 1 >= WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN  && mouse_state.x - 1 <= WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN &&  mouse_state.y + 1 >= HEIGHT ) {

                    int location_x = (mouse_state.x - (WIDTH/2 - 4*TOOL_SIZE - 4.5* MARGIN + MARGIN)) / (TOOL_SIZE + MARGIN),
                        location_y = (mouse_state.y - (HEIGHT + MARGIN)) / (TOOL_SIZE + MARGIN);
                    color = colors[location_y][location_x];
                    ciz_toolbox();
                }

                if(mouse_state.y > HEIGHT + MARGIN_2 && mouse_state.y < HEIGHT + MARGIN_2 + TOOL_SIZE_2) {
                    int i;
                    for(i=0 ; i<4 ; i++) {
                        if(mouse_state.x > WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) && mouse_state.x < WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2) {
                            switch (i) {
                            case 0:
                                mode = 3;  // draw line
                                break;
                            case 1:
                                mode = 0;  // pencil
                                break;
                            case 2:
                                if(thickness >3.0)
                                    thickness/=PHI;
                                break;
                            case 3:
                                //if(thickness < 26)
                                thickness*=PHI;
                                break;
                            }
                        }
                    }
                    for(i=0 ; i<5 ; i++) {
                        if(mouse_state.x > WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i && mouse_state.x < WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2) {
                            switch (i) {
                            case 0:
                                mode = 4;   //  rectangle
                                break;
                            case 1:
                                mode = 1;   //  triangle
                                break;
                            case 2:
                                mode = 5;   //  circle
                                break;
                            case 3:
                                mode = 2;   //  spray
                                break;
                            case 4:         //  clean drawing area
                                al_draw_filled_rectangle(0, 0, WIDTH, HEIGHT, white);
                                al_flip_display();
                                al_wait_for_vsync();
                                add_revision();
                                break;

                            }
                        }
                    }
                    if(!mouse_over) {
                        ciz_toolbox();

                    }


                }
            } else {
                // create two integers and store initial (x, y) coordinates of the mouse

                switch (mode) {
                case 0:
                    pencil();
                    add_revision();
                    break;
                case 1:
                    triangle();
                    add_revision();
                    break;
                case 2:
                    spray();
                    add_revision();
                    break;
                case 3:
                    line();
                    add_revision();
                    break;
                case 4:
                    rectangle();
                    add_revision();
                    break;
                case 5:
                    circle();
                    add_revision();
                    break;
                }

            }
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            if(event.keyboard.keycode == ALLEGRO_KEY_S) {
                al_get_keyboard_state(&keyboard_state);
                if(al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL)) {
                    save_bitmap();
                }
            } else if(event.keyboard.keycode == ALLEGRO_KEY_O) {
                al_get_keyboard_state(&keyboard_state);
                if(al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL)) {
                    open_bitmap();
                    add_revision();
                }
            } else if(event.keyboard.keycode == ALLEGRO_KEY_Z) {
                al_get_keyboard_state(&keyboard_state);
                if(al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL)) {
                    undo();
                }
            } else if(event.keyboard.keycode == ALLEGRO_KEY_Y) {
                al_get_keyboard_state(&keyboard_state);
                if(al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL)) {
                    redo();
                }
            } else if(event.keyboard.keycode == ALLEGRO_KEY_Q) {
                al_get_keyboard_state(&keyboard_state);
                if(al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL)) {
                    al_destroy_display(display);
                    return 0;
                }
            }
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            al_destroy_display(display);
            al_destroy_event_queue(queue);
            return 0;
        }

        if(mouse_over) {
            al_get_mouse_state(&mouse_state);
            if(mouse_state.y+10 > HEIGHT + MARGIN_2 && mouse_state.y-10 < HEIGHT + MARGIN_2 + TOOL_SIZE_2) {
                if(mouse_state.x+10 > WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(4) && mouse_state.x -10 < WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(1) + TOOL_SIZE_2) {
                    ciz_toolbox();
                } else if(mouse_state.x+10 > WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 && mouse_state.x -10 < WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*4 + TOOL_SIZE_2) {
                    ciz_toolbox();
                }
            }
        }

    }

    return 0;
}

/********************************************//**
 * \brief Creates a new revision.
 * \details Creates a new revision and stores previous state in another variable
 * \warning Calling in an endless loop may crash your computer
 ***********************************************/
void add_revision() {
    struct Revision *rev; // New revision to store current buffer
    rev = (struct Revision*) malloc(sizeof(struct Revision));
    rev->buffer = al_clone_bitmap(al_get_backbuffer(display)); // clone display bitmap
    rev->prev = current;
    rev->next = NULL;
    current->next = rev;
    current = rev;  // change current revision
}

/********************************************//**
 * \brief Restores previous version of the screen
 ***********************************************/
void undo() {
    if(current->prev) {
        al_draw_bitmap(current->prev->buffer, 0, 0, 0);
        current = current->prev;
        // al_flip_display();
        ciz_toolbox(); // sadece flip display kullanirsak, geri alma isleminde current color kismini da bir onceki renge boyuyor.
    }
}

/********************************************//**
 * \brief Restores next version of the screen
 ***********************************************/
void redo() {
    if(current->next) {
        al_draw_bitmap(current->next->buffer, 0, 0, 0);
        current = current->next;
        //al_flip_display();
        ciz_toolbox();
    }
}

/********************************************//**
 * \brief Draws the color palette and toolbox to the screen
 ***********************************************/
void ciz_toolbox() {
    al_draw_filled_rectangle(0, HEIGHT, WIDTH, HEIGHT + 2 * (TOOL_SIZE) + 3 * MARGIN, al_map_rgb(200, 200, 200));
    al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN, HEIGHT, WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN, HEIGHT + 2 * (TOOL_SIZE) + 3 * MARGIN, al_map_rgb(224, 224, 224));

    ALLEGRO_COLOR colors[2][8] = {{al_map_rgb(255,0,0),   al_map_rgb(255,128,0), al_map_rgb(255,255,0), al_map_rgb(128,255,0), al_map_rgb(0,255,0),   al_map_rgb(0,255,128), al_map_rgb(64,64,64),    al_map_rgb(0,0,0)},
        {al_map_rgb(0,255,255), al_map_rgb(0,128,255), al_map_rgb(0,0,255),   al_map_rgb(128,0,255), al_map_rgb(255,0,255), al_map_rgb(255,0,128), al_map_rgb(128,128,128), al_map_rgb(255,255,255)}
    };

    int i,j;
    for(i=0; i<2 ; i++) {
        for(j=0 ; j<8 ; j++) {
            al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN + MARGIN + j*(TOOL_SIZE+MARGIN),
                                     HEIGHT + MARGIN + i*(TOOL_SIZE+MARGIN),
                                     WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN + MARGIN + j*(TOOL_SIZE+MARGIN) + TOOL_SIZE,
                                     HEIGHT + MARGIN + i*(TOOL_SIZE+MARGIN) + TOOL_SIZE,
                                     colors[i][j]);
        }
    }

    //Background of the current color seciton
    al_draw_filled_rectangle(MARGIN_2,
                             HEIGHT + MARGIN_2,
                             MARGIN_2 + TOOL_SIZE_2,
                             HEIGHT + MARGIN_2 + TOOL_SIZE_2,
                             al_map_rgb(224, 224, 224));

    //Current color section
    al_draw_filled_rectangle(MARGIN_2 + MARGIN,
                             HEIGHT + MARGIN_2 + MARGIN,
                             MARGIN_2 + TOOL_SIZE_2 - MARGIN,
                             HEIGHT + MARGIN_2 + TOOL_SIZE_2 - MARGIN,
                             color);
    if(mouse_over) {
        if(mouse_state.y+1 > HEIGHT + MARGIN_2 && mouse_state.y-1 < HEIGHT + MARGIN_2 + TOOL_SIZE_2) {
            for(i=0 ; i<4 ; i++) {
                if(mouse_state.x + 1 > WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) && mouse_state.x + 1 <= WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2) {
                    al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) - 3,
                                             HEIGHT + MARGIN_2 - 3,
                                             WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2 + 3,
                                             HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                             al_map_rgb(128,128,128));
                }
            }

            for(i=0 ; i<5 ; i++) {
                if(mouse_state.x + 1 > WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i && mouse_state.x + 1 <= WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2) {
                    al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i - 3,
                                             HEIGHT + MARGIN_2 - 3,
                                             WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2 + 3,
                                             HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                             al_map_rgb(128,128,128));
                }
            }
        }
    }




    for(i=0 ; i<4 ; i++) {
        if(mode == 0 && i == 1) {
            al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        } else if(mode == 3 && i == 0) {
            al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        }
        al_draw_filled_rectangle(WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1),
                                 HEIGHT + MARGIN_2,
                                 WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(i+1) + TOOL_SIZE_2,
                                 HEIGHT + MARGIN_2 + TOOL_SIZE_2,
                                 al_map_rgb(224, 224, 224));
    }

    for(i=0 ; i<5 ; i++) {
        if(mode == 1 && i == 1) {
            al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        } else if(mode == 4 && i==0) {
            al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        } else if(mode == 5 && i==2) {
            al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        } else if(mode == 2 && i==3) {
            al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i - 3,
                                     HEIGHT + MARGIN_2 - 3,
                                     WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2 + 3,
                                     HEIGHT + MARGIN_2 + TOOL_SIZE_2 + 3,
                                     al_map_rgb(0, 0, 0));
        }
        al_draw_filled_rectangle(WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i,
                                 HEIGHT + MARGIN_2,
                                 WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*i + TOOL_SIZE_2,
                                 HEIGHT + MARGIN_2 + TOOL_SIZE_2,
                                 al_map_rgb(224, 224, 224));
    }

    ALLEGRO_BITMAP *bitmap;

    bitmap = al_load_bitmap("images/arti.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(4) + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/eksi.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(3) + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);


    bitmap = al_load_bitmap("images/kalem.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(2) + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/cizgi.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 - 4*TOOL_SIZE - 4.5 * MARGIN - (MARGIN_2 + TOOL_SIZE_2)*(1) + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/dortgen.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*0 + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/ucgen.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*1 + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/daire.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*2 + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/sprey.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*3 + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    bitmap = al_load_bitmap("images/cop.png");
    al_draw_scaled_bitmap(bitmap,0, 0,al_get_bitmap_width(bitmap),al_get_bitmap_height(bitmap),
                          WIDTH/2 + 4*TOOL_SIZE + 4.5 * MARGIN + MARGIN_2 + (TOOL_SIZE_2 + MARGIN_2)*4 + MARGIN_2/2,
                          HEIGHT + MARGIN_2 + MARGIN_2/2, TOOL_SIZE_2- MARGIN_2, TOOL_SIZE_2- MARGIN_2, 0 );
    al_destroy_bitmap(bitmap);

    al_flip_display();

    al_wait_for_vsync();

}

/********************************************//**
 * \brief Draws continuous pencil strokes until you release left mouse button
 ***********************************************/
void pencil() {
    int before_x = mouse_state.x,
        before_y = mouse_state.y;
    while(1) {
        // now until left mouse button is released, this code will run
        al_get_mouse_state(&mouse_state); // get mouse state
        // draw a filled rectangle on the (x, y) coordinates
        al_draw_filled_circle(mouse_state.x, mouse_state.y, thickness/2, color);
        // here's the clever part, if mouses coordinates are changed from before,
        // draw a line from previous coordinates to current coordinates
        // this will generate a continuous stroke
        if(mouse_state.x != before_x || mouse_state.y != before_y) {
            al_draw_line(before_x, before_y, mouse_state.x, mouse_state.y, color, thickness);
            before_x = mouse_state.x;
            before_y = mouse_state.y;
        }
        //al_flip_display(); // again, we call this for our changes to be shown
        ciz_toolbox();
        if(!al_mouse_button_down(&mouse_state, 1)) {
            // if left mouse button is not down, break the while loop
            break;
        }
    }
}

/********************************************//**
 * \brief Draws a triangle
 * \details Halts program loop until users enter three points on screen and then draws
 * a triangle using those points
 ***********************************************/
void triangle() {
    int x1, y1, x2, y2, x3, y3,x2_before, y2_before,x3_before, y3_before;
    x1 = mouse_state.x;
    y1 = mouse_state.y;
    al_draw_filled_rectangle(x1-2, y1-2, x1+2, y1+2, color);
    ciz_toolbox();
    x2_before = x1;
    y2_before = y1;
    bool flag=false;
    while(1) {
        x2 = mouse_state.x;
        y2 = mouse_state.y;
        if(x2 != x2_before && y2 != y2_before) {
            al_draw_bitmap(current->buffer, 0, 0, 0);
            x2 = mouse_state.x;
            y2 = mouse_state.y;
            al_draw_line(x1, y1, x2, y2, color, 2);
            ciz_toolbox();
            x2_before = x2;
            y2_before = y2;
        }
        al_wait_for_event(queue, &event);
        al_get_mouse_state(&mouse_state);
        if(event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            flag = true;
            break;
        }

    }
    while(flag) {
        if(x1==x2 && y1==y2)
            return;

        al_wait_for_event(queue, &event);
        al_get_mouse_state(&mouse_state);

        x3_before = x2;
        y3_before = y2;

        while(1) {
            x3 = mouse_state.x;
            y3 = mouse_state.y;
            if(x3 != x3_before && y3 != y3_before) {
                al_draw_bitmap(current->buffer, 0, 0, 0);
                x3 = mouse_state.x;
                y3 = mouse_state.y;
                al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, color);
                ciz_toolbox();
                x3_before = x3;
                y3_before = y3;
            }
            al_wait_for_event(queue, &event);
            al_get_mouse_state(&mouse_state);
            if(event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
                return;
        }

    }
}

/********************************************//**
 * \brief Fills random pixels around mouse, creating the spray tool effect
 ***********************************************/
void spray() {
    int d_x, d_y;
    while(1) {
        al_get_mouse_state(&mouse_state);
        d_x = rand() % (10 * ((int)thickness)) - (5 * ((int)thickness));
        d_y = rand() % (10 * ((int)thickness)) - (5 * ((int)thickness));

        if(thickness <5)
            al_put_pixel(mouse_state.x + d_x, mouse_state.y + d_y, color);
        else
            al_draw_filled_rectangle(mouse_state.x + d_x - 1, mouse_state.y + d_y -1, mouse_state.x + d_x +1, mouse_state.y + d_y +1, color);

        ciz_toolbox();
        if(!al_mouse_button_down(&mouse_state, 1)) {
            // if left mouse button is not down, break the while loop
            break;
        }
    }
}

void line() {
    int x1, y1, x2, y2, x2_before, y2_before;
    x1 = mouse_state.x;
    y1 = mouse_state.y;
    x2_before = x1;
    y2_before = y1;
    al_draw_filled_rectangle(x1-2, y1-2, x1+2, y1+2, color);
    ciz_toolbox();
    while(event.type != ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        x2= mouse_state.x;
        y2 = mouse_state.y;
        if(x2 != x2_before && y2 != y2_before) {
            al_draw_bitmap(current->buffer, 0, 0, 0);
            x2 = mouse_state.x;
            y2 = mouse_state.y;
            al_draw_line(x1, y1, x2, y2, color, thickness);
            ciz_toolbox();
            x2_before = x2;
            y2_before = y2;

        }
        al_wait_for_event(queue, &event);
        al_get_mouse_state(&mouse_state);
    }
}

void rectangle() {
    int x1, y1, x2, y2, x2_before, y2_before;
    x1 = mouse_state.x;
    y1 = mouse_state.y;
    x2_before = x1;
    y2_before = y1;
    al_draw_filled_rectangle(x1-2, y1-2, x1+2, y1+2, color);
    ciz_toolbox();
    while(event.type != ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        x2= mouse_state.x;
        y2 = mouse_state.y;

        if(x2 != x2_before && y2 != y2_before) {
            al_draw_bitmap(current->buffer, 0, 0, 0);
            x2 = mouse_state.x;
            y2 = mouse_state.y;
            al_draw_filled_rectangle(x1, y1, x2, y2, color);
            ciz_toolbox();
            x2_before = x2;
            y2_before = y2;
        }
        al_wait_for_event(queue, &event);
        al_get_mouse_state(&mouse_state);
    }
}

void circle() {
    int x1, y1, x2, y2,x2_before, y2_before, radius;
    x1 = mouse_state.x;
    y1 = mouse_state.y;
    x2_before = x1;
    y2_before = y1;
    al_draw_filled_circle(x1, y1, 2, color);
    ciz_toolbox();
    while(event.type != ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        x2 = mouse_state.x;
        y2 = mouse_state.y;
        if(x2 != x2_before && y2 != y2_before) {
            al_draw_bitmap(current->buffer, 0, 0, 0);
            radius = sqrt(pow(x1-x2,2) + pow(y1-y2,2));
            al_draw_filled_circle(x1,y1,radius,color);
            ciz_toolbox();
            //add_revision();

            x2_before = x2;
            y2_before = y2;
        }
        al_wait_for_event(queue, &event);
        al_get_mouse_state(&mouse_state);
    }
}


/********************************************//**
 * \brief Saves current buffer to a file
 ***********************************************/
void save_bitmap() {
    savefile = al_create_native_file_dialog((char*)al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH), "Kaydet", "", ALLEGRO_FILECHOOSER_SAVE);
    al_show_native_file_dialog(fileio, savefile);
    const char *path = al_get_native_file_dialog_path(savefile, 0);
    if(path) {
        ALLEGRO_BITMAP *bitmap = al_get_backbuffer(display);
        bitmap = al_create_sub_bitmap(bitmap, 0, 0, WIDTH, HEIGHT);
        al_save_bitmap(path, bitmap);
        al_destroy_bitmap(bitmap);
    }
}

/********************************************//**
 * \brief Opens a file and outputs it on screen
 ***********************************************/
void open_bitmap() {
    savefile = al_create_native_file_dialog((char*)al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH), "Ac", "", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
    al_show_native_file_dialog(fileio, savefile);
    const char *path = al_get_native_file_dialog_path(savefile, 0);
    if(path) {
        ALLEGRO_BITMAP *bitmap = al_load_bitmap(path);
        al_draw_bitmap(bitmap, 0, 0, 0);
        ciz_toolbox();
        al_destroy_bitmap(bitmap);
    }
}

/********************************************//**
 * \brief Logs output to console
 * \param *format String containing format information like "%d\n"
 ***********************************************/
void log_printf(char const *format, ...) {
    char str[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(str, sizeof str, format, args);
    va_end(args);
    al_append_native_text_log(textlog, "%s", str);
}
