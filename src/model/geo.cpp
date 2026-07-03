#include "model/geo.h"
#include <cmath>
static float rad(float d){ return d*(float)M_PI/180.0f; }
float haversine_km(float lat1,float lon1,float lat2,float lon2){
    float R=6371.0f, dp=rad(lat2-lat1), dl=rad(lon2-lon1);
    float a=sinf(dp/2)*sinf(dp/2)+cosf(rad(lat1))*cosf(rad(lat2))*sinf(dl/2)*sinf(dl/2);
    return 2*R*asinf(sqrtf(a));
}
float bearing_deg(float lat1,float lon1,float lat2,float lon2){
    float y=sinf(rad(lon2-lon1))*cosf(rad(lat2));
    float x=cosf(rad(lat1))*sinf(rad(lat2))-sinf(rad(lat1))*cosf(rad(lat2))*cosf(rad(lon2-lon1));
    float b=atan2f(y,x)*180.0f/(float)M_PI;
    return fmodf(b+360.0f,360.0f);
}
