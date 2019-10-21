PWDD = /home/zengzhen/other/openfec/openfec_v1_4_2_zz_pthread_version0/openfec_v1_4_2/openfec_v1.4.2/
all:
	
	gcc  redun_interface.c  queue.c redun.c  redun_openfec_aud.c preredun.c preredun_openfec_aud.c  simple_server.c -L$(PWDD)/bin/Release -lopenfec -lm -lpthread -g -o simple_server -I./ -I$(PWDD)/applis/howto_examples/simple_client_server/ -I$(PWDD)/src/lib_common 

