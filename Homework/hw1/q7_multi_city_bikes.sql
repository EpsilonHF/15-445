select bike_id, count(distinct(city)) as cnt from trip, station 
where start_station_id = station_id or end_station_id = station_id 
group by bike_id having count(distinct(city)) > 1 order by cnt desc, bike_id asc;
