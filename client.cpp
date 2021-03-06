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
		if(req->state->can_challenge && !req->state->in_challenge)
			if (this->hand_sum > this->heuristic+this->confidence) return new offer_challenge();
	}
	else {
		for (int i = 0; i < req->state->hand.size(); i++){
			this->hand_avg += req->state->hand[i];
		}
		hand_avg /= (double)req->state->hand.size();
	}

	//Issue Challenge
	if(req->state->can_challenge && !req->state->in_challenge)
	{ //If able to issue challenge and not in challenge
		if(req->state->their_points == 9 || req->state->your_tricks>=3 || (req->state->your_tricks >=2 && (req->state->your_tricks+req->state->their_tricks != req->state->total_tricks ))) return new offer_challenge();
		if (req->state->your_tricks==2 && req->state->hand[0] == 13) return new offer_challenge();
		if (hand_avg >= 11.5 && req->state->hand.size() > 1 && req->state->your_tricks >= req->state->their_tricks) return new offer_challenge();
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
			if (req->state->hand[0] == req->state->card)
				current_selected_index = 0;
			else current_selected_index = req->state->hand.size() - 1;
		}
		this->heuristic += (7-req->state->hand[current_selected_index])/104.0;
		return new play_card_response(req->state->hand[current_selected_index]);
	}
	
	else
	{ //player lead
		this->need_card = true;
		return new play_card_response(req->state->hand[req->state->hand.size() - 1]);
	}    
}

challenge_response* client::challenge(move_request* req) {
	int trickdifference = req->state->your_tricks - req->state->their_tricks;
	if(req->state->your_tricks>=3 || req->state->their_points == 9 || (req->state->your_tricks==2 && req->state->hand[0] == 13)
		|| (trickdifference==-1 && hand_avg >=11 && req->state->hand.size()==4) || (trickdifference==-1 && hand_avg >=11 && (req->state->hand.size()==2 || req->state->hand.size()==3 )))
	if(req->state->hand.size() == 5)
	{
		if(hand_avg>=11.5) return new challenge_response(true);
		else return new challenge_response(false);
	}
	else if(req->state->hand.size() == 4)
	{
		if(hand_avg>=10.5) return new challenge_response(true);
		else return new challenge_response(false);
	}
	else if(req->state->hand.size() == 3)
	{
		if(hand_avg>=10) return new challenge_response(true);
		else return new challenge_response(false);
	}
	else if(req->state->hand.size() == 2)
	{
		if(hand_avg>=11) return new challenge_response(true);
		else return new challenge_response(false);
	}
	else if(req->state->hand.size() == 1)
	{
		if(hand_avg>=11) return new challenge_response(true);
		else return new challenge_response(false);
	}
	return new challenge_response(false);
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
	this->hand_avg = 0;
}

void client::hand_done(move_result* r) {
    // increment hand count, if more than 10, reset count
	this->hand_count++;
	if (this->hand_count >= 10) this->hand_count = 0;
	this->heuristic = 35;
	this->hand_sum = 0;
	this->hand_avg = 0;
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
			myclient.confidence = 5;
			myclient.hand_avg = 0;

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
