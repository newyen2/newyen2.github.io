#include <iostream> 
#include <fstream>
#include <cstring>
#include <vector>
#include <map>
#include <utility> //pair
#include <algorithm> //find()

using namespace std;

typedef struct{
    string start;
    vector<string> symbols;
} production;
//start -> symbols

typedef struct{
    production P;
    int dot_pos;
    vector<string> next;
} item;
/*P: start -> symbols
  dot_pos(1): X.XX
  next: FOLLOW(P)
*/

typedef struct{
    int action;
    int target;
} command;
//action: 0:GOTO 1:Shift 2:Reduce/Accept 3:Error
//target: Shift/GOTO State, or Reduced Production


typedef struct{
    vector<int> state;
    vector<string> symbol;
    vector<string> input;
    command action;
} status;
//Parsing Table Line

vector<string> Split(string, char, int = -1);
//Split("AB-CD-EF-G", "-") -> ["AB","CD","EF","G"] 
//Split("AB-CD-EF-G", "-", 1) -> ["AB","CD-EF-G"] (only split once)

vector<string> Split_match(string, vector<string>, vector<string>);
//Split_match: Match by Terminal and Nonterminal
//"id+id" -> ["id","+","id"]

vector<item> Closure(vector<item>, vector<production>, vector<string>);
/*vector<item>: Based item set
  vector<production>: Production using for finding more items
  vector<string>: Terminal, for Follow()
*/

vector<string> Follow(item, vector<production>, vector<string>);
/*FOLLOW(item)
  vector<production>: Production tracks terminal
  vector<string>: Terminal, from Closure() from main()
*/

bool IsEqual(item, item, bool = false);
//Compare two items
//bool: omit item.next equal check, use for combine items both have same production but different item.next

bool IsEqual(vector<item>, vector<item>, bool = false);
//Compare two item sets
//bool: omit item.next equal check, use for combine items both have same production but different item.next

int FindEqual(vector<vector<item>>, vector<item>);
//Find the same item sets, use for combine two exactly equal item sets

void drawline(int, int);
//Grammar drawline
//int: the amount of T and N

void drawline();
//Analysis drawline

void drawspace(int, int, vector<string>);
//Grammar drawspace
//vector<string>: the context

void drawspace(vector<string>);
//Analysis drawspace
//vector<string>: the context


int main(int argc, char* argv[]){

    if(argc < 3){
        cout << "The arguments are not enough.";
        return 0;
    }  

    ifstream grammarFile;
    grammarFile.open(argv[1]);

    if(!grammarFile){
        cout << "The file is not exist.";
        return 0;
    }

    string input;
    vector<string> grammar;
    while(getline(grammarFile, input)){
        grammar.push_back(input);
    }
    // debug code: raw grammar output
    // for(string i : grammar) cout << i << "\n";

    vector<string> terminal = Split(grammar[0], ' ');
    terminal.erase(terminal.begin()); //delete "Terminal:"
    for(string &i : terminal){
        if(i.back() == ',')i.erase(i.end()-1); //erase comma
    }
    // debug code: terminal output
    // for(string i : terminal) cout << i << "\n";

    vector<string> nonterminal = Split(grammar[1], ' ');
    nonterminal.erase(nonterminal.begin());//delete "Terminal:"
    for(string &i : nonterminal){
        if(i.back() == ',')i.erase(i.end()-1);//erase comma
    }
    // debug code: nontermianl output
    // for(string i : nonterminal) cout << i << "\n";

    vector<production> productionList;
    production augmentedProduction = { // [StartSymbol]' -> [StartSymbol]
        .start = nonterminal[0] + "\'",
        .symbols = {nonterminal[0]} 
    };
    productionList.push_back(augmentedProduction);
    vector<string> subgrammar = {grammar.begin() + 2, grammar.end()}; //delete the terminal and nonterminal line
    for(string i : subgrammar){
        vector<string> splitProduction = Split(i, '-', 1); // ["S", ">A B C|D"]
        vector<string> splitSymbol;
        string start = splitProduction[0]; // ["S"]
        splitProduction[1].erase(0,1); // ["A B C|D"]
        splitProduction = Split(splitProduction[1], '|'); //divide grammar: [A B C][D]
        for(string j : splitProduction){
            splitSymbol = Split_match(j, terminal, nonterminal); //[A,B,C][D]
            productionList.push_back(*new production());
            productionList.back().start = start;
            for(string k : splitSymbol){
                productionList.back().symbols.push_back(k); //[{S}{A,B,C}] [{S}{D}]
            }
        }
    }
    //debug code: production output
    /*for(production i : productionList){
        cout << i.start << " ";
        for(string j : i.symbols){
            cout << j << " ";
        }
        cout << "\n";
    }*/

    vector<item> itemSet; //a state
    vector<vector<item>> states;
    vector<map<string, int>> edgeList; //declare A state --symbol--> B state
    pair<string, int> edge; // [symbol,destination]
    int equal_pos; 
    item start = { //AugmentedItem, base of I(0)
        .P = productionList[0],
        .dot_pos = 0,
        .next = {"$"}
    };
    itemSet.push_back(start); 
    itemSet = Closure(itemSet, productionList, terminal);
    states.push_back(itemSet); //I(0)

    vector<string> symbolList = {}; //S=T+N(not include $)
    symbolList.insert(symbolList.end(), nonterminal.begin(), nonterminal.end());
    symbolList.insert(symbolList.end(), terminal.begin(), terminal.end());
    for(int i = 0;i < states.size(); i++){
        edgeList.push_back(*new map<string, int>()); //map: edge set, n states have n maps 
        for(string j : symbolList){
            itemSet.clear();
            for(item k : states[i]){ //itemSet contains the same P.symbols[dot_pos] in a states, base of I(n)
                if(k.P.symbols[k.dot_pos] == j && k.dot_pos <k.P.symbols.size()){
                    itemSet.push_back(*new item());
                    itemSet.back().P = k.P; 
                    itemSet.back().dot_pos = k.dot_pos+1; //shift dot_pos
                    itemSet.back().next = k.next; //inherit next symbols
                }
            }
            if(!itemSet.empty()){ //no empty
                itemSet = Closure(itemSet, productionList, terminal);
                equal_pos = FindEqual(states, itemSet); //check if it has existed
                edge.first = j; // current --j--> other state
                if(equal_pos == -1){ //new
                    states.push_back(itemSet);
                    edge.second = states.size()-1;
                } else { //existed
                    edge.second = equal_pos;
                }
                edgeList.back().insert(edge);
            }
        }
    }

    vector<map<string,command>> parsingTable; //vector: n states
    command C; //[action, destination/production]
    for(map<string, int> i :edgeList){ //n edges means n slots of parsing table 
        parsingTable.push_back(*new map<string, command>());
        for(pair<string, int> j : i){
            if(find(nonterminal.begin(),nonterminal.end(), j.first) != nonterminal.end()){
                C.action = 0; // nonterminal: GOTO
            } else {
                C.action = 1; // terminal: Shift
            }
            C.target = j.second;
            parsingTable.back().insert(*new pair<string, command>(j.first,C));
        }
    }

    for(int i = 0 ; i < states.size(); i++){ // check every states
        for(item j : states[i]){ // check every item
            if(j.dot_pos == j.P.symbols.size()){ // if dot_pos is rightmost
                for(int k = 0; k<productionList.size(); k++){
                    if(productionList[k].start == j.P.start && productionList[k].symbols == j.P.symbols){
                        C.action = 2; // Reduce
                        C.target = k; // by Production[k]
                    }
                }
                for(string k : j.next){ //by all next
                    parsingTable[i].insert(*new pair<string, command>(k,C));
                }
            }
        }
    }

    //debug code: states output
    /*for(vector<item> i : states){
        for(item j : i){
            cout << j.P.start << " ";
            for(string k : j.P.symbols){
                cout << k << " ";
            }
            cout << j.dot_pos << "\n";
            for(string k : j.next){
                cout << k << " ";
            }
            cout << "\n";
        }
        cout << "\n";
    }*/

    //debug code: edge output
    /*for(int i = 0;i < edgeList.size(); i++){
        cout << i << ": ";
        for(pair<string, int> j : edgeList[i]){
            cout << j.first << "-" << j.second << " ";
        }
        cout << "\n";
    }*/

    //debug code: raw parsingdata(no table) output
    /*for(int i = 0;i < parsingTable.size(); i++){
        cout << i << ": ";
        for(pair<string, command> j : parsingTable[i]){
            cout << j.first << "-";
            switch(j.second.action){
                case 0:
                cout << "GOTO(";
                break;
                case 1:
                cout << "Shift(";
                break;
                case 2:
                cout << "Reduce(";
                break;
            }
            cout << j.second.target << ") ";
        }
        cout << "\n";
    }*/
   
    for(int i = 0; i<states.size(); i++){ //States output
        cout << i << ": ";
        for(item j : states[i]){
            cout << "[" << j.P.start << "->";
            for(int k = 0; k < j.P.symbols.size(); k++){
                if(k == j.dot_pos)cout << ".";
                cout << j.P.symbols[k];
            }
           if(j.P.symbols.size() == j.dot_pos)cout << ".";
            cout << "  {";
            for(string k : j.next){
                cout << k << ",";
            }
            cout << "}], ";
        }
        cout << "\n";
    }

    drawline(terminal.size(),nonterminal.size()); //Parsing table output
    vector<string> words = {"","$"}; // context
    string str;
    words.insert(words.end(), terminal.begin(), terminal.end());
    words.insert(words.end(), nonterminal.begin(), nonterminal.end());
    drawspace(terminal.size(), nonterminal.size(), words);
    drawline(terminal.size(),nonterminal.size());
    for(int i = 0;i < parsingTable.size(); i++){
        words.clear();
        words.push_back(to_string(i));
        str = "";
        for(pair<string, command> j : parsingTable[i]){ // $ case
            if(j.first == "$"){
                if(i == 1){ //only $ in I(1) access to "Accept"
                    str = "Acc";
                } else {
                    switch(j.second.action){
                        case 1:
                        str = "s" + to_string(j.second.target);
                        break;
                        case 2:
                        str = "r" + to_string(j.second.target);
                        break;
                    }
                }
            }
        }
        words.push_back(str);
        for(string j : terminal){ // terminal case
            str = "";
            for(pair<string, command> k : parsingTable[i]){
                if (k.first == j){
                    switch(k.second.action){
                        case 1:
                        str = "s"+to_string(k.second.target);
                        break;
                        case 2:
                        str = "r"+to_string(k.second.target);
                        break;
                    }
                }
            }
            words.push_back(str);
        }
        for(string j : nonterminal){ // nonterminal case
            str = "";
            for(pair<string, command> k : parsingTable[i]){
                if (k.first == j){
                    str = to_string(k.second.target);
                }
            }
            words.push_back(str);
        }
        drawspace(terminal.size(), nonterminal.size(), words);
        drawline(terminal.size(),nonterminal.size());
    }

    ifstream testData;
    testData.open(argv[2]);

    if(!testData){
        cout << "The file is not exist.";
        return 0;
    }

    vector<string> data;
    while(getline(testData, input)){
        data.push_back(input);
    }

    vector<status> sim; //simulation
    status current; //current parsing line
    bool isEqual;
    bool isValid;
    int currentState; //the rightmost state = the current state
    vector<string> terminalPlus = {"$"}; // T+"$"
    terminalPlus.insert(terminalPlus.end(), terminal.begin(), terminal.end());
    string targetStr; //the handle
    string copyStr;
    for(string i : data){
        cout << "String: " << i << "\n";

        copyStr = i;
        isValid = true;
        while(1){ //check if include invalid symbols
            if(copyStr == "")break;
            isValid = false;
            for(string i : terminal){
                if(copyStr.substr(0, i.size()) == i){
                    copyStr = copyStr.substr(i.size());
                    isValid = true;
                    break;
                }
            }
            if(!isValid)break; // all terminal is not match
        }
        if(!isValid){ 
            cout << "isValid: Exist Invalid Symbol.\n";
            continue;
        }

        sim.clear(); 
        current = {
            .state = {0},
            .symbol = {},
            .input = {},
        };
        for(char j : i){
            current.input.push_back(*new string(1,j));
        }
        current.input.push_back("$");

        while(1){
            for(string i : terminalPlus){ // find the handle
                isEqual = true; 
                for(int j =0; j<i.size();j++){
                    if(current.input[j][0] != i[j]){
                        isEqual = false;
                        break;
                    }
                }
                if(isEqual)targetStr = i;
            }

            currentState = current.state.back();
            if(parsingTable[currentState].count(targetStr) != 0){ // map is exist
                current.action.action = parsingTable[currentState][targetStr].action;
                current.action.target = parsingTable[currentState][targetStr].target;
            } else { // not exist
                current.action.action = 3; //ERROR
                current.action.target = 0;
            }
            sim.push_back(current);
            if(current.action.action == 2 && current.action.target == 0){ //if Accept
                isValid = true;
                break;
            }
            if(current.action.action == 3 && current.action.target == 0){ //if Error
                isValid = false;
                break;
            }
            
            switch(current.action.action){
                case 1: //Shift
                    current.state.push_back(current.action.target);//move state
                    current.input.erase(current.input.begin(), current.input.begin() + targetStr.size()); //cut handle
                    current.symbol.push_back(targetStr);//paste handle
                break;
                case 2:
                    current.state.erase(current.state.end()-productionList[current.action.target].symbols.size(),current.state.end()); //reduce n step states
                    current.symbol.erase(current.symbol.end()-productionList[current.action.target].symbols.size(),current.symbol.end()); //delete the right symbols
                    currentState = current.state.back(); //update the current
                    current.state.push_back(parsingTable[current.state.back()][productionList[current.action.target].start].target); //GOTO
                    current.symbol.push_back(productionList[current.action.target].start); //add the left start symbol
                break;
            }
        }
        cout << "isValid: "; 
        if(isValid){
            cout << "YES\n"; // Accept
        } else {
            cout << "NO\n"; // Error
        }

        drawline(); //Simulation Table
        words = {"Stack", "Symbol", "Input", "Action"};
        drawspace(words);
        drawline();
        for(status j : sim){
            words.clear();
            str = "";
            for(int k : j.state){
                str += to_string(k);
                str += " ";
            }
            words.push_back(str); // Stack output

            str = "";
            for(string k : j.symbol){
                str += k;
            }
            words.push_back(str); // Symbol output

            str = "";
            for(string k : j.input){
                str += k;
            }
            words.push_back(str); // Input output

            switch(j.action.action){ // Action output
                case 1: // Shift
                str = "s" + to_string(j.action.target);
                words.push_back(str);
                break;
                case 2:
                if(j.action.target == 0){ //Reduce + 0 = Accept, no other Reduce + 0 because S'-> S. has only one edge
                    words.push_back("Accept");
                } else { // Reduce
                    str = "r" + to_string(j.action.target);
                    words.push_back(str);
                }
                break;
                case 3: // Error
                words.push_back("Error");
                break;
            }
            drawspace(words);
            drawline();
        }
    }
}

vector<string> Split(string str, char keyword, int count){
    vector<string> split_str;
    string substr = "";
    for(char i : str){
        if(i == keyword && count){
            if(substr != ""){
                split_str.push_back(substr);
                substr = "";
                count--;
            }
            continue;
        }
        substr += i;
    }
    if(substr != "")split_str.push_back(substr);
    return split_str;
}

vector<string> Split_match(string str, vector<string> T, vector<string> N){
    vector<string> splitSymbol;
    vector<string> symbolList = {};
    symbolList.insert(symbolList.end(), T.begin(), T.end());
    symbolList.insert(symbolList.end(), N.begin(), N.end());
    string substr = "";
    for(char i : str){
        substr += i;
        for(string j : symbolList){
            if(substr.find(j) != string::npos){
                splitSymbol.push_back(substr);
                substr = "";
                break;
            }
        }
    }
    return splitSymbol;
}

vector<item> Closure(vector<item> items, vector<production> P, vector<string> T){
    string keySymbol;
    item descendant; // the item is created by other item
    int equal_pos;
    for(int i = 0; i < items.size(); i++){
        if(items[i].dot_pos < items[i].P.symbols.size()){ //E->.T
            keySymbol = items[i].P.symbols[items[i].dot_pos]; 
            for(production j : P){
                if(j.start == keySymbol){ 
                    descendant.P = j; //T->???
                    descendant.dot_pos = 0; //T->.???
                    if(items[i].dot_pos+1 < items[i].P.symbols.size()){ //if E->.TF
                        descendant.next = Follow(items[i], P, T); //Follow(F)
                    } else {
                        descendant.next = items[i].next;
                    }
                    for(int k=0; k<items.size(); k++){
                        equal_pos = -1;
                        if(IsEqual(descendant, items[k], true)){ // conbine the items which have same production but different next in a closure
                            equal_pos = k;
                            break;
                        }
                    }
                    if(equal_pos == -1){
                        items.push_back(descendant);
                    } else {
                        if(!(descendant.next == items[equal_pos].next)){
                            for(string k : descendant.next){
                                if(find(items[equal_pos].next.begin(),items[equal_pos].next.end(), k) == items[equal_pos].next.end()){
                                    items[equal_pos].next.push_back(k);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return items;
}

vector<string> Follow(item I, vector<production> P, vector<string> T){
    vector<string> next_pos;
    bool isExist;
    next_pos.push_back(I.P.symbols[I.dot_pos+1]); //add all symbols which is next to the position
    for(int i = 0; i < next_pos.size(); i++){
        for(production j : P){
            if(j.start == next_pos[i]){
                isExist = false;
                for(string k : next_pos){
                    if(k == j.symbols[0]){
                        isExist = true;
                        break;
                    }
                }
                if(!isExist){
                    next_pos.push_back(j.symbols[0]);
                }
            }
        }
    }
    vector<string> nextSymbol = {};
    for(string i : next_pos){
        for(string j : T){ //just save the terminal
            if(i == j){
                nextSymbol.push_back(i);
            }
        }
    }
    return nextSymbol;
}

bool IsEqual(item A, item B, bool omitNext){
    if(!(A.P.start == B.P.start))return false;
    if(!(A.P.symbols == B.P.symbols))return false;
    if(A.dot_pos != B.dot_pos)return false;
    if(!(A.next == B.next) && !omitNext)return false;
    return true;
}

bool IsEqual(vector<item> A, vector<item> B, bool omitNext){
    if(A.size() != B.size())return false;
    bool isItemEqual;
    for(item i : A){
        isItemEqual = false;
        for(item j : B){
            if(IsEqual(i, j, omitNext)){
                isItemEqual = true;
                break;
            }
        }
        if(!isItemEqual)return false;
    }
    for(item i : B){
        isItemEqual = false;
        for(item j : A){
            if(IsEqual(i, j, omitNext)){
                isItemEqual = true;
                break;
            }
        }
        if(!isItemEqual)return false;
    }
    return true;
}

int FindEqual(vector<vector<item>> S, vector<item> itemSet){
    int equal_pos = -1;
    for(int i = 0;i < S.size(); i++){
        if(IsEqual(S[i], itemSet)){
            equal_pos = i;
            break;
        }
    }
    return equal_pos;
}

void drawline(int T, int N){
    cout << "+-----+-----";
    for(int i =0 ;i < T; i++){
        cout << "+";
        cout << "-----";
    }
    for(int i =0 ;i < N; i++){
        cout << "+";
        cout << "----";
    }
    cout << "+\n";
}

void drawline(){
    cout << "+----------------------------------------------------------------";
    cout << "+----------------------------------";
    cout << "+----------------------------------";
    cout << "+--------------------+\n";
}

void drawspace(int T, int N , vector<string> words){
    int count = 0;
    for(int i = 0 ;i < T+2; i++){
        cout << "|";
        cout << words[count];
        for(int j = 0; j <= 4-words[count].size(); j++) cout << " ";
        count++;
    }
    for(int i = 0 ;i < N; i++){
        cout << "|";
        cout << words[count];
        for(int j = 0; j <= 3-words[count].size(); j++) cout << " ";
        count++;
    }
    cout << "|\n";
}

void drawspace(vector<string> S){
    cout << "|";
    cout << S[0];
    for(int j = 0; j <= 63-S[0].size(); j++) cout << " ";
    cout << "|";
    cout << S[1];
    for(int j = 0; j <= 33-S[1].size(); j++) cout << " ";
    cout << "|";
    cout << S[2];
    for(int j = 0; j <= 33-S[2].size(); j++) cout << " ";
    cout << "|";
    cout << S[3];
    for(int j = 0; j <= 19-S[3].size(); j++) cout << " ";
    cout << "|\n";
}
