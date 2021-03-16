#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <wininet.h> // requires -lwininet to be added to linker options
#include <cmath>

using namespace std;

#define earth 6371
#define radius 7221
#define earthrot 359
#define LKlat 55.930171
#define LKlng 37.518219
#define range 2468

struct coord
{
    double lat = 0;
    double lng = 0;
};

ostream & operator << (ostream& os, coord& a)
{
    os << a.lat << ", " << a.lng;;
    return os;
}

struct timendate
{
    int day = 0;
    int month = 0;
    int year = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

ostream & operator << (ostream& os, timendate& a)
{
    os << a.day << "." << a.month << "." << a.year << " " << a.hour << ":" << a.minute << ":" << a.second;
    return os;
}

istream & operator >> (istream& is, timendate& a)
{
    cout << "Set the day" << endl;
    is >> a.day;

    cout << "Set the month" << endl;
    is >> a.month;

    cout << "Set the year" << endl;
    is >> a.year;

    cout << "Set the hour" << endl;
    is >> a.hour;

    cout << "Set the minute" << endl;
    is >> a.minute;

    cout << "Set the second" << endl;
    is >> a.second;

    return is;
}

double azimuth(coord& k)
{
    double az = 0;
    if ( (k.lat > LKlat && k.lng > LKlng) || (k.lat > LKlat && k.lng < LKlng) )
    {
        az = (-1) * (180.f/3.14) * atan( sin( (3.14/180.f) * (LKlng - k.lng) ) /
                                 ( (cos((3.14/180.f) * LKlat) * tan( (3.14/180.f) * k.lat) ) - (sin((3.14/180.f) * LKlat) * cos((3.14/180.f) * (LKlng - k.lng)) ) )  );
    }

    else if (k.lat < LKlat && k.lng < LKlng)
    {
        az = 180 + (-1) * (180.f/3.14) * atan( sin( (3.14/180.f) * (LKlng - k.lng) ) /
                                 ( (cos((3.14/180.f) * LKlat) * tan( (3.14/180.f) * k.lat) ) - (sin((3.14/180.f) * LKlat) * cos((3.14/180.f) * (LKlng - k.lng)) ) )  );
    }

    else if (k.lat < LKlat && k.lng > LKlng)
    {
        az = 180 - (180.f/3.14) * atan( sin( (3.14/180.f) * (LKlng - k.lng) ) /
                                 ( (cos((3.14/180.f) * LKlat) * tan( (3.14/180.f) * k.lat) ) - (sin((3.14/180.f) * LKlat) * cos((3.14/180.f) * (LKlng - k.lng)) ) )  );
    }
    else if (k.lat == LKlat)
    {
        if (k.lng > LKlng)
        {
            return 90;
        }
        else
        {
            return 270;
        }
    }

    else if (k.lng == LKlng && k.lat < LKlat)
    {
        return 180;
    }

    if(az < 0)
    {
        az += 360;
    }

    return az;
}

double date_difference(timendate& a, timendate& b)
{
    double res = 0;
    res += b.day - a.day;
    res += 30.f * (b.month - a.month);
    res += 365.f * (b.year - a.year);
    res += (1.f/24.f) * (b.hour - a.hour);
    res += (1.f/1440.f) * (b.minute - a.minute);
    res += (1.f/86400.f) * (b.second - a.second);
    return res;
}

timendate date_sum(timendate & a, timendate & b)
{
    timendate f;

    f.day = a.day + b.day;
    if (f.day > 30)
    {
        f.month++;
        f.day -= 30;
    }

    f.month = a.month + b.month;

    f.year = a.year + b.year;
    if (f.month > 12)
    {
        f.year++;
        f.month -= 12;
    }

    f.hour = a.hour + b.hour;
    if (f.hour > 23)
    {
        f.day++;
        f.hour -= 24;
    }

    f.minute = a.minute + b.minute;
    if (f.minute > 59)
    {
        f.hour++;
        f.minute -= 60;
    }

    f.second = a.second + b.second;
    if (f.second > 59)
    {
        f.minute++;
        f.second -= 60;
    }

    return f;
}

timendate date_time_sum(timendate & a, double time)
{
    timendate s;

    while (time > 1)
    {
        time--;
        s.day++;
    }

    while (time > (1.f/24.f))
    {
        time -= (1.f/24.f);
        s.hour++;
    }

    while (time > (1.f/1440.f))
    {
        time -= (1.f/1440.f);
        s.minute++;
    }

    while (time > (1.f/86400.f))
    {
        time -= (1.f/86400.f);
        s.second++;
    }

    return date_sum(a, s);
}

void downloadFile(const char* WEB_URL)
{
    ofstream fout("1.txt");
    HINTERNET WEB_CONNECT = InternetOpenA("Default_User_Agent", 0, 0, 0, 0);
    HINTERNET WEB_ADDRESS = InternetOpenUrlA(WEB_CONNECT, WEB_URL, NULL, 0, INTERNET_FLAG_KEEP_CONNECTION, 0);
    char* _DATA_RECIEVED = new char[1024];
    DWORD NO_BYTES_READ = 0;
    while (InternetReadFile(WEB_ADDRESS, _DATA_RECIEVED, 1024, &NO_BYTES_READ) && (NO_BYTES_READ))
    {
        fout << _DATA_RECIEVED << endl;
    }
    InternetCloseHandle(WEB_ADDRESS);
    InternetCloseHandle(WEB_CONNECT);
}

coord get_noaa_coords(double time, double vel, double angle)
{
    angle =  (3.14 * angle) / 180.f;
    double late = 0;
    double longi = 2.08;

    double alpha = vel * time;
    double mem = alpha;
    int i = 0;

    while (mem >  90)
    {
        mem -= 90;
        i++;
    }

    alpha = (3.14 * alpha) / 180.f;
    late -= asin( sin(alpha) * sin(angle));

    if (i % 4 == 1 || i % 4 == 2)
    {
        longi -= 180 - (180.f / 3.14) * asin( (sin(alpha) * cos(angle)) / cos(late) ) - earthrot * time;
    }

    else
    {
        longi -= (180.f / 3.14) * asin( (sin(alpha) * cos(angle)) / cos(late) ) - earthrot * time;
    }

    late *= (180.f / 3.14);


    while (longi < -360)
    {
        longi += 360;
    }

    while (longi > 360)
    {
        longi -= 360;
    }

    if (longi < -180)
    {
        longi += 360;
    }

    if( longi > 180)
    {
        longi -= 360;
    }

    coord out;
    out.lat = (-1)*late;
    out.lng =  (-1) *longi;

    return out;
}

int main()
{
    double coef1 = 7221.f / 6371.f;
    double coef2 = 7221.f - 6371.f;

    double xLK = earth * cos((LKlat * 3.14) / 180.f) * cos((LKlng * 3.14) / 180);
    double yLK = earth * cos((LKlat * 3.14) / 180.f) * sin((LKlng * 3.14) / 180);
    double zLK = earth * sin((LKlat * 3.14) / 180.f);

    timendate start;
    start.day = 14;
    start.month = 3;
    start.year = 2021;
    start.hour = 21;
    start.minute = 29;
    start.second = 40;

    cout.fixed;
    cout.precision(4);

    cout << "Download TLE? y/n" << endl;
    char answer;
    cin >> answer;
    if (answer == 'y'){
            downloadFile("http://celestrak.com/NORAD/elements/noaa.txt");
    }

    ifstream file;
    file.open("1.txt");

    string tle[3];
    bool check = 1;
    while (check)
    {
        string str;
        getline(file, str);
        if (str.find("NOAA 19 [+]") != string::npos){
            tle[0] = str;
            getline(file, tle[1]);
            getline(file, tle[2]);
            check = 0;
        }
    }

    file.close();

    double frequency = 0;
    for (int i = 1; i > -10; i--)
    {
        if (i > -1)
        {
            frequency += pow(10, i) * (tle[2][53 - i] - 48);
        }
        else if (i < -1)
        {
            frequency += pow(10, i + 1) * (tle[2][53 - i] - 48);
        }
    }

    char* angle = new char[9];
    for (int i = 9; i < 18; i++)
    {
        if (i != 11)
        {
            angle[i - 9] = tle[2][i];
        }
        else
        {
            angle[i - 9] = '.';
        }
    }

    double inclination = atof(angle);
    delete[] angle;

    cout << endl << "Select the start of the time span" << endl << endl;
    timendate first_date;
    cin >> first_date;
    cout << endl << "Selected date: " << first_date << endl << endl;

    cout << "Select the end of the time span" << endl << endl;
    timendate last_date;
    cin >> last_date;
    cout << endl << "Selected date: " << last_date << endl << endl;;


    double angvelocity = 360.f * frequency;

    ofstream foutput;
    foutput.open("out.csv");

    double span = date_difference(start, last_date);
    bool sight = 0;
    double rmin = 3000;
    double tmin = 0;
    coord cmin;
    int i = 0;

    for (double t = date_difference(start, first_date); t < span;)
    {
        coord k;
        t += (1.f / 10.f) * 1.f/1440.f;

        k = get_noaa_coords(t, angvelocity, inclination);
        foutput << k << endl;

        double x = radius * cos((k.lat * 3.14) / 180.f) * cos((k.lng * 3.14) / 180);
        double y = radius * cos((k.lat * 3.14) / 180.f) * sin((k.lng * 3.14) / 180);
        double z = radius * sin((k.lat * 3.14) / 180.f);
        double r = sqrt( pow(x - xLK, 2) + pow(y - yLK, 2) + pow(z - zLK, 2) );

        if (r < range && sight == 0)
        {
            rmin = r;
            tmin = t;
            cmin = k;
            i++;
            sight = 1;
            timendate e;
            e = date_time_sum(start, t);

            cout << i << '.' << " In sight: " << e << " azimuth: " << azimuth(k) << endl;
        }

        else if (r < range && sight == 1)
        {
            if (r < rmin)
            {
                rmin = r;
                tmin = t;
                cmin = k;
            }
        }

        else if (r >= range && sight == 1)
        {
            double lowside = sqrt( (1.f - pow((rmin / coef2), 2))  / ( ( (0.25 * pow((coef1 - 1), 2) ) / pow(rmin, 2) ) - ( ( 0.25 * pow((coef1 + 1), 2) ) / pow(coef2, 2) )) );
            double alpha = acos( (lowside * (coef1 + 1)) / (2 * rmin) );
            double beta = acos( (lowside * (coef1 - 1)) / (2 * coef2) );
            double elev = ((alpha + beta) / 3.14) * 180 - 90;
            timendate e, m;
            e = date_time_sum(start, t);
            m = date_time_sum(start, tmin);
            cout <<"   MAX elevation: " << elev  << " at time: " << m  << " azimuth: " << azimuth(cmin) << endl <<  "   Out of sight:" << e << " azimuth: " << azimuth(k) << endl << endl;

            sight = 0;
            rmin = 2000;
            tmin = 0;
            cmin.lat = 0;
            cmin.lng = 0;
        }
    }

    cout << "Use Plot.m to see trajectory in MATLAB" << endl;

    foutput.close();
    system("pause");
    return 0;
}
