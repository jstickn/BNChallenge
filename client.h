#include <random>
#include <string>

#include "game_io/game_io.h"
#include "game_io/json_response.h"
#include "game_io/move_request.h"
#include "game_io/challenge_response.h"
#include "game_io/offer_challenge.h"
#include "game_io/play_card_response.h"
#include "game_io/greeting.h"
#include "game_io/game_result.h"

using namespace std;

class client : public game_player {
    public:
        client() : random_challenge(0,1) {}
        virtual void error(error_msg*);
        virtual move_response* move(move_request*);
        virtual challenge_response* challenge(move_request*);
        virtual void server_greeting(greeting*);
        virtual void trick_done(move_result*);
        virtual void hand_done(move_result*);
        virtual void game_over(game_result*);

		double heuristic;
		bool need_card;
		int hand_count;
		bool first_of_hand;
		double hand_sum;
		double confidence;
		double hand_avg;
    private:
        default_random_engine random_generator;
        uniform_int_distribution<int> random_challenge;
};
