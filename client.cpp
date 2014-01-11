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
	if(req->state->opp_lead == true)
	{ //opponent lead
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
		{ //If no card is found, throwaway the lowest value card
			for(int i=req->state->hand.size()-1; i>=0; i--)
			{ //Iterate through hand to find lowest card that beats opponent lead
				if(req->state->hand[i] < current_selected_val)
				{
					current_selected_val = req->state->hand[i];
					current_selected_index = i;
				}
			}
		}
		return new play_card_response(req->state->hand[current_selected_index]);
	}
	
	else
	{ //player lead
		return new play_card_response(req->state->hand[0]);
	}    
}

challenge_response* client::challenge(move_request* req) {
	if(req->state->your_tricks>=3)
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
    // left blank for you
}

void client::trick_done(move_result* r) {
    // left blank for you
}

void client::hand_done(move_result* r) {
    // left blank for you
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
