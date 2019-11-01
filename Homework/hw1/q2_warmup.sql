select city, count(*) as cnt from station 
group by city order by cnt asc, city asc;
