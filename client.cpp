#ifndef _MSC_VER
#include <unistd.h>
#define SLEEP(x) sleep(x)
#else
#include <windows.h>
#define SLEEP(x) Sleep(x)
#endif

#include "client.h"
#include "json_socket/json_socket.h"

void client::error(error_msg* err) {
    cout << "error: " << err->message << endl;
}

move_response* client::move(move_request* req) {
	if (this->first_of_hand) {
		for (int i = 0; i < 4; i++){
			this->hand_sum += req->state->hand[i]; //purposely leave out lowest 
		}
		this->first_of_hand = false;
		if (this->hand_sum > this->heuristic+this->confidence) return new offer_challenge();
	}

	//Issue Challenge
	if(req->state->can_challenge && !req->state->in_challenge)
	{ //If able to issue challenge and not in challenge
		if(req->state->their_points == 9 || req->state->your_tricks>=3 || (req->state->your_tricks >=2 && (req->state->your_tricks+req->state->their_tricks != req->state->total_tricks ))) return new offer_challenge();
	}

	//Play Card
	if(req->state->opp_lead == true)
	{ //opponent lead
		this->heuristic += (7 - req->state->card)/104.0;
		this->need_card = false;

		bool card_found = false;
		int current_selected_val = 100;
		int current_selected_index = -1;
		
		for(int i=req->state->hand.size()-1; i>=0; i--)
		{ //Iterate through hand to find lowest card that beats opponent lead
			if(req->state->hand[i] > req->state->card && req->state->hand[i] < current_selected_val)
			{
				current_selected_val = req->state->hand[i];
				current_selected_index = i;
				card_found = true;
			}
		}
		
		if(card_found == false)
		{ //If no card is found, throw away the lowest value card
			for(int i=req->state->hand.size()-1; i>=0; i--)
			{ //Iterate through hand to find lowest card that beats opponent lead
				if(req->state->hand[i] < current_selected_val)
				{
					current_selected_val = req->state->hand[i];
					current_selected_index = i;
				}
			}
		}
		this->heuristic += (7-req->state->hand[current_selected_index])/104.0;
		return new play_card_response(req->state->hand[current_selected_index]);
	}
	
	else
	{ //player lead
		this->need_card = true;
		return new play_card_response(req->state->hand[0]);
	}    
}

challenge_response* client::challenge(move_request* req) {
	if(req->state->your_tricks>=3 || req->state->their_points == 9|| req->state->your_tricks > req->state->their_tricks || this->hand_sum > this->confidence + heuristic)
	{
		return new challenge_response(true);
	}
	else
	{
		return new challenge_response(false);
	}
}

void client::server_greeting(greeting* greet) {
    cout << "Connected to server." << endl;
}

void client::game_over(game_result* r) {
    this->heuristic = 35;
}

void client::trick_done(move_result* r) {
    if (this->need_card)
		this->heuristic += (7 - r->card)/104.0;
}

void client::hand_done(move_result* r) {
    // increment hand count, if more than 10, reset count
	this->hand_count++;
	if (this->hand_count >= 10) this->hand_count = 0;
	this->heuristic = 35;
	this->hand_sum = 0;
	this->first_of_hand = true;
}

int main(void) {
#ifdef _MSC_VER
	// Initialize Winsock
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif

    string server = "cuda.contest";
    for (;;) {
        try {
            json_socket contest_server = json_socket(server, "9999");

            client myclient = client();
			myclient.heuristic = 35;
			myclient.hand_count = 0;
			myclient.confidence = 10;

            game_mediator game = game_mediator(&myclient, &contest_server);
            game.start();
        }
        catch (UnableToConnect) {
            cout << "Unable to connect. Waiting 10 seconds..." << endl;
        }
        catch (NotParseableJson) {
            cout << "Unparsable JSON encountered. Server probably hung up. Waiting 10 seconds..."
                 << endl;
        }
        SLEEP(10);
    }

    return 0;
}
