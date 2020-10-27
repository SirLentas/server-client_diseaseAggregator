all: worker master whoClient whoServer

worker: ./src/worker.c ./structs/file_list.c ./structs/summary_list.c ./structs/records.c
	gcc -g3 -o worker ./src/worker.c ./structs/file_list.c ./structs/summary_list.c ./structs/records.c

master: ./src/master.c ./structs/country_list.c ./structs/summary_list.c
	gcc -g3 -o master ./src/master.c ./structs/country_list.c ./structs/summary_list.c

whoClient: ./src/whoClient.c
	gcc -g3 -o whoClient ./src/whoClient.c -pthread

whoServer:	./src/whoServer.c ./structs/summary_list.c
	gcc -g3 -o whoServer ./src/whoServer.c ./structs/summary_list.c -pthread

clean:
	rm -f master worker whoServer whoClient

clean_up:
	rm -r -f ./error_reports/*