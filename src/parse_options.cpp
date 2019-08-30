#include <ofs_io.h>

using namespace std;

string check_pos(string s)
{
    return s.substr(0,s.find(":"));
}

string find_pos(string s)
{
    return s.substr((s.find("\"")+1),(s.rfind("\"")-s.find("\"")-1));
}

void parse_data(thread_data *td)
{
    ifstream file("../paragram.txt");
    string line;
    if(file){
        while(getline(file,line))
        {
            if(check_pos(line) == "diskname")
            {
                td->disk = find_pos(line);
            }
            else if(check_pos(line) == "direct")
            {
                td->direct = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "iodepth")
            {
                td->iodepth = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "rw")
            {
                td->rw = find_pos(line);
            }
            else if(check_pos(line) == "rw_ratio")
            {
                td->rw_ratio = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "bs")
            {
                td->bs = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "filesize")
            {
                td->filesize = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "num_threads")
            {
                td->num_threads = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "runtime")
            {
                td->runtime = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "iodepth_low")
            {
                td->iodepth_low = atoi(find_pos(line).c_str());
            }
            else if(check_pos(line) == "io_size")
            {
                td->iosize = atoi(find_pos(line).c_str());
            }
            else
            {
                cout<<"no such paragram"<<endl;
            }
        }
    }
    else{
        cout<<"error: no paragram file."<<endl;
    }
    return;
}