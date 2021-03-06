#include <iostream>
#include <memory>
#include <vector>

#include <board_generator.h>
#include <config_parser.h>
#include <curses.h>
#include <game.h>

#include "colors.cpp"

int main() {
    int min_board_width = 30;
    int max_board_width = 100;
    bool start_game = false;

    // Reading data from config.txt and saving them
    std::unique_ptr<Config_Parser> parser(new Config_Parser);
    parser->parse_and_store_data(); 

    int board_length = stoi(parser->config_data["length"]);
    int board_width = stoi(parser->config_data["width"]);
    
    // ncurses window for display   
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    my_display::initiate_colors();

    // validating user input on board dimension
    if(board_width < min_board_width || board_width > max_board_width || (board_width / board_length) != 2) {
        attron(A_BOLD | A_UNDERLINE);
        printw("Please maintain Board dimension rules dictated below\n");
        attroff(A_BOLD | A_UNDERLINE); 
        printw("%d >= Board Width >= %d\n", max_board_width, min_board_width);
        printw("Width/Height ratio = 2\n");
        printw("Modify Board dimension in config.txt");
        getch();
        return endwin();
    }

    if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}

    //Initial messages AKA instructions
    int r, c;
    attron(A_BOLD | A_UNDERLINE);
    printw("Dodge The Blocks\n\n");
    attroff(A_BOLD | A_UNDERLINE);
    printw(" ");
    getyx(stdscr, r, c);
    my_display::draw_player(r, c);
    printw(" = Player\n\n ");
    getyx(stdscr, r, c);
    my_display::draw_obstacle(r, c);
    printw(" = Incoming blocks\n\n");
    printw("Use ARROW KEYs to avoid block and up your score\n");
    printw("Board dimension can be changed from config.txt\n");
    printw("Heads up: Difficulty level increases as score goes up\n");
    printw("Press \"S\" to start and any key to cancel");
    int ch = getch();
    if(ch == 83 || ch == 115) {
        start_game = true;
    } else {
        return endwin();
    }

    if(start_game) {
        // Init Board with width & length
        std::unique_ptr<Board_Generator> board(new Board_Generator(board_length, board_width));

        // Init Game 
        std::unique_ptr<Game> game(new Game(board_length, board_width, std::move(board)));
        game->load_game(); // init inner matrix or board
        game->launch_game(); // launching both vehicle and obstacle thread
        
        return endwin();
    }    
}