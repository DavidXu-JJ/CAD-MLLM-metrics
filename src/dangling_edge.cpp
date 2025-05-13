#include "dirent.h"
#include "fstream"
#include "queue"
#include "sys/stat.h"
#include "sys/types.h"
#include "thread"
#include "unistd.h"
#include "unordered_map"

#include "geometrycentral/surface/manifold_surface_mesh.h"
#include "geometrycentral/surface/meshio.h"
#include "geometrycentral/surface/vertex_position_geometry.h"

#include "geometrycentral/surface/direction_fields.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

#include "args/args.hxx"
#include "imgui.h"

using namespace geometrycentral;
using namespace geometrycentral::surface;

std::string get_parent_path(const std::string &filepath) {
  size_t found = filepath.find_last_of("/\\");
  return (found != std::string::npos) ? filepath.substr(0, found) : "";
}

std::string get_filename(const std::string &filepath) {
  size_t found = filepath.find_last_of("/\\");
  return (found != std::string::npos) ? filepath.substr(found + 1) : filepath;
}

std::string replace_extension(const std::string &filename,
                              const std::string &new_extension) {
  size_t found = filename.find_last_of(".");
  return (found != std::string::npos)
             ? filename.substr(0, found) + new_extension
             : filename + new_extension;
}

void create_directories(const std::string &dirPath) {
  if (mkdir(dirPath.c_str(), 0755) && errno != EEXIST) {
    std::cerr << "Error creating directory: " << dirPath << std::endl;
  }
}

bool isStlFile(const std::string &filename) {
  return filename.rfind(".stl") == (filename.size() - 4);
}

void replaceSubstring(std::string &str, const std::string &from,
                      const std::string &to) {
  size_t startPos = str.find(from);
  if (startPos != std::string::npos) {
    str.replace(startPos, from.length(), to);
  }
}

std::vector<std::string> list_directory(const std::string &dirPath) {
  std::vector<std::string> filenames;
  DIR *dir = opendir(dirPath.c_str());

  if (dir == nullptr) {
    std::cerr << "Error opening directory: " << dirPath << std::endl;
    return filenames;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string filename(entry->d_name);
    if (filename != "." && filename != "..") {
      filenames.push_back(filename);
    }
  }

  closedir(dir);
  return filenames;
}

// Written by Jingwei Xu for https://arxiv.org/abs/2411.04954
void computeDanglingEdge(std::vector<std::string> &stlFiles, size_t start,
                         size_t end) {

  for (size_t iter = start; iter < end; ++iter) {
    std::string inputFilename = stlFiles[iter];

    // == Geometry-central data
    std::unique_ptr<ManifoldSurfaceMesh> manifold_mesh;
    std::unique_ptr<SurfaceMesh> surface_mesh;
    std::unique_ptr<VertexPositionGeometry> geometry;
    bool use_manifold = true;
    // Load mesh
    try {
      std::tie(manifold_mesh, geometry) = readManifoldSurfaceMesh(inputFilename);
    } catch (const std::runtime_error &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cout << "Manifold sanity check failed for .stl, try loading .ply"
                << std::endl;
      use_manifold = false;
      replaceSubstring(inputFilename, ".stl", ".ply");
      try {
          std::tie(surface_mesh, geometry) = readSurfaceMesh(inputFilename);
      } catch (const std::runtime_error &err) {
          std::cerr << "Error: " << e.what() << std::endl;
          std::cout << "Failed loading mesh." << std::endl;
          return;
      }
    }

    try {
        std::unique_ptr<geometrycentral::surface::SurfaceMesh> mesh;
        if (use_manifold) {
            mesh = std::move(manifold_mesh);
        } else {
            mesh = std::move(surface_mesh);
        }
        std::unordered_map<long long, int> edge_weight;
        for (Halfedge e : mesh->interiorHalfedges()) {
            // do science here
            size_t firstVert = e.edge().firstVertex().getIndex();
            size_t secondVert = e.edge().secondVertex().getIndex();
            long long lowerVert =
                static_cast<long long>(std::min(firstVert, secondVert));
            long long higherVert =
                static_cast<long long>(std::max(firstVert, secondVert));
            edge_weight[(lowerVert * (1ll << 31)) + higherVert] += 1;
        }

        size_t vertNum = mesh->nVertices();
        Vector3 max_point{-1e-9, -1e-9, -1e-9};
        Vector3 min_point{1e-9, 1e-9, 1e-9};
        for (size_t i = 0; i < vertNum; ++i) {
            Vector3 current_point = geometry->inputVertexPositions[i];
            for (int dim = 0; dim < 3; ++dim) {
                max_point[dim] = std::max(max_point[dim], current_point[dim]);
                min_point[dim] = std::min(min_point[dim], current_point[dim]);
            }
        }
        double scale = -1.f;
        for (int dim = 0; dim < 3; ++dim) {
            scale = std::max(scale, max_point[dim] - min_point[dim]);
        }
        scale /= 2.0f;
        std::cout << scale << std::endl;
        if (scale < 0.f) {
            std::cerr << "Error: normalized scale less than 0." << std::endl;
            return;
        }

        double danglingEdgeLength = 0.0f;
        std::unordered_map<size_t, size_t> index_map;
        std::vector<glm::vec3> nodes;
        std::vector<std::array<size_t, 2>> edges;
        size_t count = 0;
        for (const auto &pair : edge_weight) {
            if (pair.second == 1) {
                size_t firstVertIndex =
                    static_cast<size_t>(pair.first & ((1ll << 31) - 1ll));
                size_t secondVertIndex = static_cast<size_t>((pair.first >> 31));

                Vector3 vert1 = geometry->inputVertexPositions[firstVertIndex];
                Vector3 vert2 = geometry->inputVertexPositions[secondVertIndex];
                double edgeLength = norm(vert1 - vert2);
                danglingEdgeLength += edgeLength;

                if (!index_map.count(firstVertIndex)) {
                    index_map[firstVertIndex] = count;
                    count += 1;
                    nodes.emplace_back(glm::vec3(vert1.x, vert1.y, vert1.z));
                }
                if (!index_map.count(secondVertIndex)) {
                    index_map[secondVertIndex] = count;
                    count += 1;
                    nodes.emplace_back(glm::vec3(vert2.x, vert2.y, vert2.z));
                }
                std::array<size_t, 2> edge {index_map[firstVertIndex], index_map[secondVertIndex]};
                edges.emplace_back(edge);
            }
        }
        danglingEdgeLength /= scale;

        // Create output directory path and filename
        std::string inputPath = inputFilename;
        std::string outputDir = get_parent_path(inputPath) + "_dangling_edge";
        std::string outputFilename =
            outputDir + "/" + replace_extension(get_filename(inputPath), ".txt");

        // Create output directory if it doesn't exist
        create_directories(outputDir);

        std::ofstream outFile(outputFilename);
        if (outFile.is_open()) {
            outFile << danglingEdgeLength;
            outFile.close();
            std::cout << "Dangling Edge Length saved to: " << outputFilename
                << std::endl;
        } else {
            std::cerr << "Error: Could not open file " << outputFilename
                << " for writing." << std::endl;
        }

        std::string outputEdgeDir = get_parent_path(inputPath) + "_dangling_edge_describe";
        std::string outputEdgeFilename =
                outputEdgeDir + "/" + replace_extension(get_filename(inputPath), ".txt");

        // Create output directory if it doesn't exist
        create_directories(outputEdgeDir);
        std::ofstream outEdgeFile(outputEdgeFilename);
        if (outEdgeFile.is_open()) {
            outEdgeFile << nodes.size() << std::endl;
            for (size_t nodeIter = 0; nodeIter < nodes.size(); ++nodeIter) {
                outEdgeFile
                << nodes[nodeIter][0] << ' '
                << nodes[nodeIter][1] << ' '
                << nodes[nodeIter][2] << std::endl;
            }

            outEdgeFile << edges.size() << std::endl;
            for (size_t edgeIter = 0; edgeIter < edges.size(); ++edgeIter) {
                outEdgeFile << edges[edgeIter][0] << ' ' << edges[edgeIter][1] << std::endl;
            }
            outEdgeFile.close();
            std::cout << "Dangling Edge Description saved to: " << outputEdgeFilename
                      << std::endl;
        } else {
            std::cerr << "Error: Could not open file " << outputEdgeFilename
                      << " for writing." << std::endl;
        }
    }  catch (const std::runtime_error &err) {
        std::cerr << "Error: " << err.what() << std::endl;
        std::cout << "Failed computing." << std::endl;
        return;
    }
  }
}

int main(int argc, char **argv) {

  // Configure the argument parser
  args::ArgumentParser parser("geometry-central & Polyscope example project");
  args::Positional<std::string> inputDirname(parser, "mesh_dir",
                                             "Directory contains mesh files.");

  // Parse args
  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help &h) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // Make sure a mesh name was given
  if (!inputDirname) {
    std::cerr << "Please specify a mesh file as argument" << std::endl;
    return EXIT_FAILURE;
  }

  std::string dirPath = args::get(inputDirname);

  std::vector<std::string> files = list_directory(dirPath);

  dirPath.push_back('/');
  std::vector<std::string> stlFiles;
  for (std::string s : files) {
    if (isStlFile(s)) {
      stlFiles.push_back(dirPath + s);
    }
  }

  size_t numThreads = std::thread::hardware_concurrency();
  size_t batch = stlFiles.size() / numThreads + 1;
  std::vector<std::thread> workers;
  for (size_t i = 0; i < numThreads; ++i) {
    size_t start = i * batch;
    size_t end = std::min(start + batch, stlFiles.size());
    workers.push_back(
        std::thread(computeDanglingEdge, std::ref(stlFiles), start, end));
  }

  for (size_t i = 0; i < numThreads; ++i) {
    if (workers[i].joinable()) {
      workers[i].join();
    }
  }

  return EXIT_SUCCESS;
}
