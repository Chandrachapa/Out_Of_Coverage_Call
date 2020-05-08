void waiting_time (double own_snr, double loc_accuracy, double other_device_snr, bool incoverage, double mySoc, double T_cycle ){
double result;
if (mySoc < 20)(
result = 1/(loc_accuracy +0.1)*(own_snr/other_device_snr)*(1/(incoverage+0.1))*(20/(mySoc))*T_cycle  
);
else if (
result = 1/(loc_accuracy +0.1)*(own_snr/other_device_snr)*(1/(incoverage+0.1))*(20/20)*T_cycle
);
}