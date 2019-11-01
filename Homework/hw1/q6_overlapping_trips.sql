select t1.bike_id, t1.id, t1.start_time, t1.end_time, t2.id, t2.start_time, t2.end_time 
from trip as t1, trip as t2 where t1.bike_id = t2.bike_id 
and t1.bike_id between 100 and 200 and t1.id < t2.id 
and t1.start_time < t2.end_time and t2.start_time < t1.end_time 
order by t1.bike_id asc, t1.id asc, t2.id asc;
