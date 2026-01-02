/**
 * World Map Viewer - Shows all areas as nodes with connections between them
 * Uses directional positioning based on cross-area exit directions
 */

let cy;
let worldData = null;

// Initialize the map
async function init() {
    try {
        const response = await fetch('data/world_graph.json');
        worldData = await response.json();

        updateStats();
        initCytoscape();
        setupEventListeners();
    } catch (error) {
        console.error('Error loading world data:', error);
        document.getElementById('stats').textContent = 'Error loading data: ' + error.message;
    }
}

function updateStats() {
    const totalRooms = worldData.nodes.reduce((sum, n) => sum + n.room_count, 0);
    document.getElementById('stats').textContent =
        `${worldData.nodes.length} areas | ${totalRooms} rooms | ${worldData.edges.length} connections`;
}

function initCytoscape() {
    // Convert data to Cytoscape format
    const elements = [];

    // Add area nodes with coordinates
    for (const node of worldData.nodes) {
        elements.push({
            data: {
                id: node.id,
                label: node.name,
                roomCount: node.room_count,
                filename: node.filename,
                vnumRange: node.vnum_range,
                x: node.x || 0,
                y: node.y || 0,
            }
        });
    }

    // Add edges between areas
    const edgeMap = new Map();
    for (const edge of worldData.edges) {
        // Create unique edge ID
        const edgeId = `${edge.from}-${edge.to}`;
        const reverseId = `${edge.to}-${edge.from}`;

        // Check if reverse edge exists
        const hasReverse = worldData.edges.some(e =>
            e.from === edge.to && e.to === edge.from
        );

        // Only add if we haven't added the reverse
        if (!edgeMap.has(reverseId)) {
            elements.push({
                data: {
                    id: edgeId,
                    source: edge.from,
                    target: edge.to,
                    connectionCount: edge.connection_count,
                    oneWayCount: edge.one_way_count,
                    direction: edge.direction || 'unknown',
                    bidirectional: hasReverse,
                }
            });
            edgeMap.set(edgeId, true);
        }
    }

    cy = cytoscape({
        container: document.getElementById('cy'),
        elements: elements,
        style: [
            {
                selector: 'node',
                style: {
                    'label': 'data(label)',
                    'background-color': '#4a90d9',
                    'color': '#fff',
                    'text-valign': 'bottom',
                    'text-halign': 'center',
                    'font-size': '9px',
                    'text-wrap': 'wrap',
                    'text-max-width': '80px',
                    'text-margin-y': 5,
                    'width': 'mapData(roomCount, 1, 200, 25, 60)',
                    'height': 'mapData(roomCount, 1, 200, 25, 60)',
                    'border-width': 2,
                    'border-color': '#357abd',
                }
            },
            {
                selector: 'node:selected',
                style: {
                    'background-color': '#5cb85c',
                    'border-color': '#4cae4c',
                }
            },
            {
                selector: 'edge',
                style: {
                    'width': 'mapData(connectionCount, 1, 10, 1, 4)',
                    'line-color': '#555',
                    'curve-style': 'straight',
                    'target-arrow-shape': 'triangle',
                    'target-arrow-color': '#555',
                    'arrow-scale': 0.8,
                }
            },
            {
                selector: 'edge[?bidirectional]',
                style: {
                    'target-arrow-shape': 'none',
                    'line-color': '#666',
                }
            },
            {
                selector: 'edge[oneWayCount > 0]',
                style: {
                    'line-style': 'dashed',
                    'line-color': '#f0ad4e',
                    'target-arrow-color': '#f0ad4e',
                }
            },
            {
                selector: '.dimmed',
                style: {
                    'opacity': 0.15,
                    'label': '',
                }
            },
        ],
        // Use preset layout with coordinates from data
        layout: {
            name: 'preset',
            positions: function(node) {
                const scale = 120;  // Scale factor for spacing
                return {
                    x: (node.data('x') || 0) * scale,
                    y: -(node.data('y') || 0) * scale,  // Flip Y so north is up
                };
            },
        },
        minZoom: 0.1,
        maxZoom: 5,
        wheelSensitivity: 0.3,
    });

    // Fit to view
    cy.fit(50);

    // Node click handler
    cy.on('tap', 'node', function(evt) {
        const node = evt.target;
        showAreaInfo(node.data());
    });

    // Background click to clear selection
    cy.on('tap', function(evt) {
        if (evt.target === cy) {
            document.getElementById('area-info').innerHTML =
                '<p>Click an area node to see details.</p>';
        }
    });
}

function showAreaInfo(data) {
    // Get connections for this area from the edges
    const connections = [];
    cy.edges().forEach(edge => {
        const source = edge.data('source');
        const target = edge.data('target');
        const direction = edge.data('direction');
        const count = edge.data('connectionCount');
        const bidirectional = edge.data('bidirectional');

        if (source === data.id) {
            const targetNode = cy.getElementById(target);
            const targetName = targetNode.data('label') || target;
            const arrow = bidirectional ? '↔' : '→';
            connections.push(`${direction}: ${arrow} ${targetName} (${count})`);
        } else if (target === data.id) {
            const sourceNode = cy.getElementById(source);
            const sourceName = sourceNode.data('label') || source;
            const reverseDir = {north:'south', south:'north', east:'west', west:'east', up:'down', down:'up'}[direction] || direction;
            const arrow = bidirectional ? '↔' : '←';
            connections.push(`${reverseDir}: ${arrow} ${sourceName} (${count})`);
        }
    });

    const connectionsHtml = connections.length > 0
        ? connections.map(c => `<div class="connection-item">${c}</div>`).join('')
        : '<em>No cross-area connections</em>';

    const html = `
        <p><strong>${data.label}</strong></p>
        <span class="info-label">Filename</span>
        <span class="info-value">${data.filename}</span>

        <span class="info-label">VNUM Range</span>
        <span class="info-value">${data.vnumRange[0]} - ${data.vnumRange[1]}</span>

        <span class="info-label">Room Count</span>
        <span class="info-value">${data.roomCount}</span>

        <span class="info-label">Connections to other areas</span>
        <div class="connections-list">${connectionsHtml}</div>

        <a href="area.html?area=${data.id}" class="view-area-btn">View Area Map</a>
    `;
    document.getElementById('area-info').innerHTML = html;
}

function setupEventListeners() {
    // Search functionality
    const searchInput = document.getElementById('search-input');
    const searchBtn = document.getElementById('search-btn');

    function doSearch() {
        const query = searchInput.value.toLowerCase().trim();
        if (!query) {
            cy.nodes().removeClass('dimmed');
            cy.edges().removeClass('dimmed');
            return;
        }

        cy.nodes().forEach(node => {
            const matches = node.data('label').toLowerCase().includes(query) ||
                           node.data('id').toLowerCase().includes(query);
            if (matches) {
                node.removeClass('dimmed');
            } else {
                node.addClass('dimmed');
            }
        });
    }

    searchBtn.addEventListener('click', doSearch);
    searchInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') doSearch();
    });
    searchInput.addEventListener('input', () => {
        if (!searchInput.value) {
            cy.nodes().removeClass('dimmed');
            cy.edges().removeClass('dimmed');
        }
    });
}

// Start the app
document.addEventListener('DOMContentLoaded', init);
