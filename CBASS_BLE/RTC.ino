const char monthsOfTheYear[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* NOTE: The String class has the reputation of creating memory leaks.                                     
 *  Using it a few times, such as at the start of a run, is probably fine,
 *  but it should not be used in the main loop.
 */
String getdate()
{
   t = rtc.now();
   String dateStr =  String(t.year(), DEC) + "_" + String(monthsOfTheYear[t.month()-1])  + "_" + String(t.day(), DEC);
   //String dateStr =  String(t.year());
   return dateStr;
}
String gettime()
{
   t = rtc.now();
   String tStr =  String(t.hour(), DEC) + ":" 
       + String((t.minute() < 10) ? "0" : "")  // Zero-pad minutes
       + String(t.minute(), DEC)  + ":" 
       + String((t.second() < 10) ? "0" : "")  // Zero-pad seconds
       + String(t.second(), DEC);
   return tStr;
}


void checkTime()
{
  t = rtc.now();
  Serial.print("Time: ");
  Serial.print(t.hour(), DEC);
  Serial.print(":");
  if (t.minute() < 10) Serial.print(0);
  Serial.println(t.minute(), DEC);
  return;
}



void clockInit() {
  while (!rtc.begin()) {
    Serial.println("Couldn't start RTC");
    delay(2000);
  }
  Serial.println("Started RTC");
}
