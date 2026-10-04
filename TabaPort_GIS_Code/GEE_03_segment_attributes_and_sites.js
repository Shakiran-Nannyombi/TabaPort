/**************************************************************************
 * TabaPort GEE 03: Road segment attributes, AHP exposure inputs and
 * candidate sensor crossings. Output GeoJSON loads straight into
 * Sharif's Leaflet layer and joins road_segment_state on segment_id.
 * Inputs uploaded from QGIS (OSM Uganda extract, split at 100 m):
 *   kampala_road_segments  (LineString, property segment_id e.g. clock_tower_01)
 *   kampala_hospitals      (Point, from OSM amenity=hospital, checked in field)
 **************************************************************************/
var aoi = ee.Geometry.Rectangle([32.49, 0.22, 32.68, 0.41]);
var segments  = ee.FeatureCollection('projects/tabaport/assets/kampala_road_segments');
var hospitals = ee.FeatureCollection('projects/tabaport/assets/kampala_hospitals');
var fsiStack  = ee.Image('projects/tabaport/assets/tabaport_fsi_hand_30m'); // export of GEE 01

// Distance to nearest hospital (m), searched to 5 km
var hospDist = hospitals.distance({searchRadius: 5000, maxError: 10}).rename('hosp_dist_m');

// People within 500 m (WorldPop 100 m, 2020)
var pop = ee.ImageCollection('WorldPop/GP/100m/pop')
  .filter(ee.Filter.eq('country', 'UGA')).filter(ee.Filter.eq('year', 2020)).first();
var pop500 = pop.reduceNeighborhood({reducer: ee.Reducer.sum(),
  kernel: ee.Kernel.circle(500, 'meters')}).rename('pop_500m');

// Drainage distance again for crossing detection
var upa = ee.Image('MERIT/Hydro/v1_0_1').select('upa');
var distDrain = upa.gt(1).selfMask().fastDistanceTransform(100).sqrt()
  .multiply(90).rename('dist_drain').reproject({crs: 'EPSG:32636', scale: 90});

var stack = fsiStack.select(['fsi', 'hand'])
  .addBands(hospDist).addBands(pop500).addBands(distDrain);

var attrs = stack.reduceRegions({
  collection: segments,
  reducer: ee.Reducer.mean().combine({reducer2: ee.Reducer.min(), sharedInputs: true}),
  scale: 30
}).map(function (f) {
  // Static AHP exposure inputs, scaled 0 to 1 for the dispatch engine
  var infra = ee.Number(1).subtract(ee.Number(f.get('hosp_dist_m_min')).divide(5000)).max(0);
  var popn  = ee.Number(f.get('pop_500m_mean')).divide(40000).min(1);
  return f.set({
    infra_score: infra, pop_score: popn,
    low_point: ee.Number(f.get('hand_min')).lt(3),
    static_class: ee.Algorithms.If(ee.Number(f.get('fsi_mean')).gt(0.7), 'high',
                  ee.Algorithms.If(ee.Number(f.get('fsi_mean')).gt(0.5), 'medium', 'low'))
  });
});

// Candidate sensor crossings: high susceptibility AND the road crosses a drainage line
var candidates = attrs
  .filter(ee.Filter.gt('fsi_mean', 0.7))
  .filter(ee.Filter.lt('dist_drain_min', 30))
  .sort('fsi_mean', false);
print('Candidate crossings for Eric (top 25):', candidates.limit(25));

Map.addLayer(attrs, {color: '555555'}, 'Segments');
Map.addLayer(candidates, {color: 'd73027'}, 'Candidate sensor crossings');

Export.table.toDrive({collection: attrs, description: 'road_segments_gis',
  folder: 'TabaPort_GIS', fileFormat: 'GeoJSON'});
Export.table.toDrive({collection: candidates, description: 'candidate_sensor_crossings',
  folder: 'TabaPort_GIS', fileFormat: 'GeoJSON'});
