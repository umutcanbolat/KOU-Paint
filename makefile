build: main.c images
	@gcc main.c -o KOU\ Paint `pkg-config --libs allegro-5 allegro_primitives-5 allegro_image-5 allegro_color-5 allegro_dialog-5` -lm
	@echo "installation completed"

run: KOU\ Paint
	@echo "running.. "
	@./KOU\ Paint
