select city_cnt.city, round(city_cnt.cnt * 1.0 / total_cnt.cnt, 4) as ratio from
(select city, count(*) as cnt from trip, station
where trip.start_station_id = station.station_id or trip.start_station_id = station.station_id
group by city) as city_cnt,
(select count(*) as cnt from trip) as total_cnt
order by ratio desc, city_cnt.city asc;
