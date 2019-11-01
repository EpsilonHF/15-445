with dates as (select date(start_time) as tdate from trip union select
date(end_time) as tdate from trip) select tdate,
round(sum(strftime('%s', min(datetime(end_time), datetime(tdate, '+1
day'))) - strftime('%s', max(datetime(start_time), datetime(tdate)))) *
1.0 / (select count(distinct(bike_id)) from trip where bike_id <= 100),
4) as avg_duration from trip, dates where bike_id <= 100 and
datetime(start_time) < datetime(tdate, '+1 day') and datetime(end_time) >
datetime(tdate) group by tdate order by avg_duration desc limit 10;
