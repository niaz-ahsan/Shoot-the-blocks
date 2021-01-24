#include <chrono>

#include <game.h>
#include <curses.h>
#include <pthread.h>

/*
Board value:
0 - Empty cell
1 - Obstacle (-)
2 - Vehicle (*)
*/

void Game::load_game() {
    // initialize inner vector
    std::vector<int> inner;
    for (int i = 0; i < _row; i++) {
        inner.clear();
        for (int j = 0; j < _col; j++) {
            inner.push_back(0);
        }
        _inner_board.push_back(inner);
    }
}

void Game::launch_game() {
    _board->draw_board();

    // generating and moving obstacle continuously
    std::future<void> obstacle_thread = std::async(std::launch::async, &Game::generate_obstacles, this);

    // creating my vehicle thread
    std::future<void> vehicle_thread = std::async(std::launch::async, &Game::move_my_vehicle, this);
    

    // returns once game is over
    vehicle_thread.wait();
    obstacle_thread.wait();

    // display text after game is over
    post_game_over();
}

void Game::move_my_vehicle() {
    int vehicle_row = _row-1;
    int vehicle_col = _col/2;   
    change_inner_board_value(vehicle_row, vehicle_col, 2); 
    _board->update_cell(vehicle_row, vehicle_col, 2, -2, -2);
    
    // notifying obstacle generation thread to create obstacle
    if(!vehicle_created) {
        std::unique_lock<std::mutex> locker(_mutex);
        vehicle_created = true;
        _cv.notify_all();
    }

    int pressed_key;
    while(game_should_go_on) {
        pressed_key = getch();
        switch (pressed_key) {
            case KEY_UP:
                if(vehicle_row > 0) { 
                    change_inner_board_value(vehicle_row, vehicle_col, 0);
                    vehicle_row -= 1;
                    if(check_collision_from_vehicle(vehicle_row, vehicle_col)) {
                        stop_game();
                        break;
                    }
                    change_inner_board_value(vehicle_row, vehicle_col, 2);
                    int prev_row = vehicle_row + 1; 
                    int prev_col = vehicle_col;
                    _board->update_cell(vehicle_row, vehicle_col, 2, prev_row, prev_col);
                }
                break;
            case KEY_LEFT:
                if(vehicle_col > 0) {
                    change_inner_board_value(vehicle_row, vehicle_col, 0);
                    vehicle_col -= 1;
                    if(check_collision_from_vehicle(vehicle_row, vehicle_col)) {
                        stop_game();
                        break;
                    }
                    change_inner_board_value(vehicle_row, vehicle_col, 2);
                    int prev_row = vehicle_row; 
                    int prev_col = vehicle_col + 1;
                    _board->update_cell(vehicle_row, vehicle_col, 2, prev_row, prev_col);
                }       
                break;
            case KEY_DOWN:
                if(vehicle_row < _row - 1) {
                    change_inner_board_value(vehicle_row, vehicle_col, 0);
                    vehicle_row += 1;
                    if(check_collision_from_vehicle(vehicle_row, vehicle_col)) {
                        stop_game();
                        break;
                    }
                    change_inner_board_value(vehicle_row, vehicle_col, 2);
                    int prev_row = vehicle_row - 1; 
                    int prev_col = vehicle_col;
                    _board->update_cell(vehicle_row, vehicle_col, 2, prev_row, prev_col);
                }    
                break;  
            case KEY_RIGHT:
                if(vehicle_col < _col - 1) {
                    change_inner_board_value(vehicle_row, vehicle_col, 0);
                    vehicle_col += 1;
                    if(check_collision_from_vehicle(vehicle_row, vehicle_col)) {
                        stop_game();
                        break;
                    }
                    change_inner_board_value(vehicle_row, vehicle_col, 2);
                    int prev_row = vehicle_row; 
                    int prev_col = vehicle_col - 1;
                    _board->update_cell(vehicle_row, vehicle_col, 2, prev_row, prev_col);
                }    
                break;    
            default:
                break;
        }
    } 
}

void Game::generate_obstacles() {
    while(!vehicle_created) {
        // wait till vehicle isn't created
        std::unique_lock<std::mutex> locker(_mutex);
        _cv.wait(locker);
    }   

    while(game_should_go_on) {
        std::thread obs(&Game::generate_single_obstacle, this);
        obs.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void Game::generate_single_obstacle() {
    int row = 0;
    std::vector<int> cols;
    // init vector with all cols
    for(int c = 0; c < _col; c++) {
        cols.push_back(c);
    }
    // changing values & updating the cells
    change_inner_board_value(row, cols, 1);
    _board->update_cells(row, cols, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    for(int r = 0; r < _row; r++) {    
        if((r + 1) < _row) {
            change_inner_board_value(r+1, cols, 1);
            _board->update_cells(r + 1, cols, 1);
        }
        change_inner_board_value(r, cols, 0);
        _board->update_cells(r, cols, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

bool Game::check_collision_from_obstacle(int row, int col) {
    if(get_inner_board_cell(row, col) == 2) { // obstacle found vehicle in this position
        return true;
    }
    return false;
}

bool Game::check_collision_from_vehicle(int row, int col) {
    if(get_inner_board_cell(row, col) == 1) { // vehicle found obstacle in this position
        return true;
    }
    return false;
} 

int Game::get_obstacle_delay() {
    if(score <= 5) {
        obstacle_delay = 200;
    } else if(score > 5 && score <= 10) {
        obstacle_delay = 160;
    } else if(score > 10 && score <= 20) {
        obstacle_delay = 120;
    } else if(score > 20 && score <= 30) {
        obstacle_delay = 80;
    } else {
        obstacle_delay = 50;
    } 
    return obstacle_delay;
}

int Game::get_obstacle_gap() {
    if(score <= 10) {
        obstacle_max_gap = 6; 
    } else if(score > 10 && score <= 20) {
        obstacle_max_gap = 5;
    } else if(score > 20 && score <= 30) {
        obstacle_max_gap = 4;
    } else if(score > 30 && score <= 40) {
        obstacle_max_gap = 3;
    } else  {
        obstacle_max_gap = 2;
    }
    return obstacle_max_gap;
}

void Game::stop_game() {
    std::lock_guard<std::mutex> locker(_mutex);
    game_should_go_on = false;
}

void Game::change_inner_board_value(int b_row, int b_col, int val) {
    std::lock_guard<std::mutex> locker(_mutex);
    _inner_board[b_row][b_col] = val;
}

void Game::change_inner_board_value(int row, int val) {
    std::lock_guard<std::mutex> locker(_mutex);
    for(int c = 0; c < _col; c++) {
        _inner_board[row][c] = val;
    }
}

void Game::change_inner_board_value(int row, std::vector<int> &cols, int val) {
    std::lock_guard<std::mutex> locker(_mutex);
    for(int c = 0; c < cols.size(); c++) {
        _inner_board[row][cols.at(c)] = val;
    }
}

int Game::get_inner_board_cell(int row, int col) {
    return _inner_board[row][col];
}

void Game::post_game_over() {
    clear();
    attron(A_STANDOUT);
    printw("GAME OVER!\n");
    attroff(A_STANDOUT);
    attron(A_UNDERLINE);
    printw("Your score: %d\n", score);
    attroff(A_UNDERLINE);
    attron(A_DIM);
    printw("Press SPACE to exit!");
    attroff(A_DIM);
    int inp = getch();
    while(inp != 32) {  
        inp = getch(); 
    }
}

// dummy method for testing the matrix
void Game::print_inner_board() {
    std::lock_guard<std::mutex> locker(_mutex);
    for(int row=0; row<_row; row++) {
        for(int col=0; col<_col; col++) {
            mvwprintw(_win, row+30, col, "%d", _inner_board[row][col]);
        }
        printw("\n");
    }
    wrefresh(_win);
}