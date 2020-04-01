#ifndef _RTC_H_
#define _RTC_H_

/* Complete register description can be found in m6242b_oki_datasheet.pdf */

struct msm6242b {
  uint32_t : 28, second2 : 4;
  uint32_t : 28, second1 : 4;
  uint32_t : 28, minute2 : 4;
  uint32_t : 28, minute1 : 4;
  uint32_t : 28, hour2 : 4;
  uint32_t : 28, hour1 : 4;
  uint32_t : 28, day2 : 4;
  uint32_t : 28, day1 : 4;
  uint32_t : 28, month2 : 4;
  uint32_t : 28, month1 : 4;
  uint32_t : 28, year2 : 4;
  uint32_t : 28, year1 : 4;
  uint32_t : 28, weekday : 4;
  uint32_t : 28, control1 : 4;
  uint32_t : 28, control2 : 4;
  uint32_t : 28, control3 : 4;
};

extern struct msm6242b volatile msm6242b;

#endif /* !_RTC_H_ */
