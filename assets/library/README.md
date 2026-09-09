# RoadSafe AR Asset Library

This directory is the local model library scanned by the native RoadSafe AR Asset Library.

Recommended folders:

- `vehicles`
- `road_infrastructure`
- `evidence`
- `environment`
- `people`
- `street_furniture`
- `forensic_markers`
- `imported`

RoadSafe scans recursively for `.glb` and `.gltf` files.

## Metadata sidecar

Place a file with the same stem and `.roadsafeasset` extension next to the model.

Example:

```text
asset_id=roadsafe.vehicle.example
name=Example Vehicle
category=Vehicles
source_url=https://example.com/model
author=Example Author
license=CC0 1.0
attribution=No attribution required
material_profile=Metallic-Roughness PBR
meters_per_unit=1.0
ar_ready=true
```

The 3D model is a visualization asset. Its realism does not make it forensic evidence.