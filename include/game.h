#ifndef GAME_H
#define GAME_H

#include <vector>
#include <curses.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>

#include "board_generator.h"

class Game {
public: 
    Game(int r, int c, WINDOW *win, std::unique_ptr<Board_Generator> board) : _row(r-2), _col(c-2), _win(win), _board(std::move(board)) {}
    ~Game() { delwin(_win); }
    void load_game();
    void launch_game();
    void print_inner_board(); // dummy method for test
private:
    std::vector<std::vector<int> > _inner_board;
    int _row;
    int _col;
    int score = 0;
    WINDOW *_win;
    int obstacle_delay;  // inscreases to get higher speed
    int obstacle_max_gap; // gap for vehicle to pass through
    std::unique_ptr<Board_Generator> _board;
    std::mutex _mutex;
    //std::mutex _mutex_shooting;
    std::condition_variable _cv;
    //std::condition_variable _cv_shooting;
    bool game_should_go_on = true; 
    bool vehicle_created = false; // obstacle generation should wait until player/vehicle is created... this var controls that
    //bool shot_fired = false;
    void move_my_vehicle();
    void generate_obstacles();
    void generate_single_obstacle();
    void generate_player_shot(int, int);
    void change_inner_board_value(int, int, int);// change a single value
    void change_inner_board_value(int, int); // change values of a whole row
    void change_inner_board_value(int, std::vector<int>&, int); // change values of a row specified by cols
    int get_inner_board_cell(int, int);
    void post_game_over();
    bool check_collision_from_vehicle(int, int);
    bool check_collision_from_obstacle(int, int);
    bool check_collision_from_bullet_to_obstacle(int, int);
    void action_after_bullet_collides_with_obstacle(int, int);
    int check_collision_from_obstacle_to_bullet(int, std::vector<int>&); // returns the col where it might collide with the bullet in the specific row
    void action_after_obstacle_collides_with_bullet(int, int, std::vector<int>&);
    void stop_game();
    int get_obstacle_delay();
    int get_obstacle_gap();
};

#endif