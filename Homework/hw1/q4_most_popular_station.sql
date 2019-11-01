select station_cnt.city, station_cnt.station_name, station_cnt.cnt from
(select city, station_name, count(*) as cnt, 
rank() over(partition by city order by count(*) desc) as rank
from trip, station
where trip.start_station_id = station.station_id or trip.start_station_id = station.station_id
group by city, station_name) as station_cnt where station_cnt.rank = 1 order by city asc;
