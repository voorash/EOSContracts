eosio-cpp -o latenitetoken/latenitetoken.wasm latenitetoken/latenitetoken.cpp

cleos wallet unlock -n jungletesting --password YOURWALLETPASSWORD[ONLY USE THIS ON TESTNETS]

cleos -u http://jungle.cryptolions.io:18888 set contract YOURACCOUNT YOURCONTRACTCODEFOLDERNAME -p YOURACCOUNT@active -c
cleos -u http://jungle.cryptolions.io:18888 set contract YOURACCOUNT YOURCONTRACTCODEFOLDERNAME -p YOURACCOUNT@active
