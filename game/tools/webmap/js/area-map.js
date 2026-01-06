/**
 * Area Map Viewer - Shows detailed room layout within a single area
 */

let cy;
let areasData = null;
let currentArea = null;
let currentZLevel = null; // null = show all
let currentTextSize = 8; // default font size in pixels
let currentWrapWidth = 60; // default wrap width in pixels
let showWarpedEdges = false; // hide warped connections by default

// Sector colors matching merc.h sector types
const SECTOR_COLORS = {
    0: '#808080',  // inside
    1: '#FFD700',  // city
    2: '#90EE90',  // field
    3: '#228B22',  // forest
    4: '#8B4513',  // hills
    5: '#A0522D',  // mountain
    6: '#4169E1',  // water_swim
    7: '#00008B',  // water_noswim
    9: '#87CEEB',  // air
    10: '#F4A460', // desert
};

const DIR_NAMES = ['North', 'East', 'South', 'West', 'Up', 'Down'];

// Initialize the map
async function init() {
    try {
        const response = await fetch('data/areas.json');
        areasData = await response.json();

        populateAreaSelector();
        setupEventListeners();

        // Check URL for area parameter
        const urlParams = new URLSearchParams(window.location.search);
        const areaParam = urlParams.get('area');
        if (areaParam && areasData[areaParam]) {
            document.getElementById('area-select').value = areaParam;
            loadArea(areaParam);
        }
    } catch (error) {
        console.error('Error loading area data:', error);
    }
}

function populateAreaSelector() {
    const select = document.getElementById('area-select');
    select.innerHTML = '<option value="">Select an area...</option>';

    // Sort areas by name
    const sortedAreas = Object.entries(areasData)
        .sort((a, b) => a[1].name.localeCompare(b[1].name));

    for (const [areaId, area] of sortedAreas) {
        const option = document.createElement('option');
        option.value = areaId;
        option.textContent = `${area.name} (${area.room_count} rooms)`;
        select.appendChild(option);
    }
}

function loadArea(areaId) {
    currentArea = areasData[areaId];
    if (!currentArea) return;

    // Update URL
    history.replaceState(null, '', `?area=${areaId}`);

    // Reset Z-level
    currentZLevel = null;
    document.getElementById('z-level').textContent = 'All';

    initCytoscape();
    updateAreaStats();
}

function initCytoscape() {
    const elements = [];
    const rooms = currentArea.rooms;

    // Collect all Z levels
    const zLevels = new Set();

    // Add room nodes
    for (const [vnum, room] of Object.entries(rooms)) {
        const z = room.coords?.z || 0;
        zLevels.add(z);

        elements.push({
            data: {
                id: `room-${vnum}`,
                vnum: parseInt(vnum),
                label: room.name,
                sector: room.sector,
                sectorName: room.sector_name,
                deadEnd: room.dead_end,
                x: room.coords?.x || 0,
                y: room.coords?.y || 0,
                z: z,
                exits: room.exits,
            }
        });
    }

    // Add edges for exits and portal nodes for cross-area connections
    const addedEdges = new Set();
    const addedPortals = new Set();
    for (const [vnum, room] of Object.entries(rooms)) {
        for (const [dir, exit] of Object.entries(room.exits)) {
            const destVnum = exit.to;

            // Check if destination is in this area
            if (rooms[destVnum]) {
                // Internal connection
                const edgeId = `${vnum}-${destVnum}`;
                const reverseId = `${destVnum}-${vnum}`;

                // Check if reverse exit exists
                const destRoom = rooms[destVnum];
                const reverseDir = [2, 3, 0, 1, 5, 4][parseInt(dir)];
                const hasReverse = destRoom.exits[reverseDir]?.to === parseInt(vnum);

                // For bidirectional, only add one edge
                if (hasReverse && addedEdges.has(reverseId)) continue;

                elements.push({
                    data: {
                        id: `edge-${edgeId}`,
                        source: `room-${vnum}`,
                        target: `room-${destVnum}`,
                        direction: parseInt(dir),
                        oneWay: exit.one_way,
                        warped: exit.warped,
                        door: exit.door,
                        bidirectional: hasReverse && !exit.one_way,
                    }
                });
                addedEdges.add(edgeId);
            } else {
                // Cross-area connection - add portal node
                const portalId = `portal-${vnum}-${dir}`;
                if (addedPortals.has(portalId)) continue;

                // Find which area this leads to
                let destAreaId = null;
                let destAreaName = 'Unknown Area';
                for (const [areaId, area] of Object.entries(areasData)) {
                    if (area.rooms && area.rooms[destVnum]) {
                        destAreaId = areaId;
                        destAreaName = area.name;
                        break;
                    }
                }

                // Position portal node adjacent to the source room
                // Find an unoccupied position
                const sourceRoom = room;
                const baseX = sourceRoom.coords?.x || 0;
                const baseY = sourceRoom.coords?.y || 0;
                const baseZ = sourceRoom.coords?.z || 0;

                // Direction offsets for initial placement attempt
                const dirOffsets = [
                    [0, 1],   // N
                    [1, 0],   // E
                    [0, -1],  // S
                    [-1, 0],  // W
                    [0.5, 0.5],   // U (diagonal)
                    [-0.5, -0.5]  // D (diagonal)
                ];
                const [dx, dy] = dirOffsets[parseInt(dir)];

                // Try to find unoccupied position, extending outward if needed
                let portalX = baseX + dx;
                let portalY = baseY + dy;
                let attempts = 0;
                const maxAttempts = 5;

                // Helper to check if position is occupied (with tolerance for floating point)
                const isOccupied = (px, py, pz) => {
                    const tolerance = 0.1;
                    // Check rooms
                    for (const [rv, rdata] of Object.entries(rooms)) {
                        const rx = rdata.coords?.x || 0;
                        const ry = rdata.coords?.y || 0;
                        const rz = rdata.coords?.z || 0;
                        if (Math.abs(rx - px) < tolerance &&
                            Math.abs(ry - py) < tolerance &&
                            Math.abs(rz - pz) < tolerance) {
                            return true;
                        }
                    }
                    // Check other portals
                    for (const el of elements) {
                        if (el.data?.isPortal) {
                            if (Math.abs(el.data.x - px) < tolerance &&
                                Math.abs(el.data.y - py) < tolerance &&
                                Math.abs(el.data.z - pz) < tolerance) {
                                return true;
                            }
                        }
                    }
                    return false;
                };

                while (attempts < maxAttempts && isOccupied(portalX, portalY, baseZ)) {
                    // Extend further in the same direction
                    attempts++;
                    portalX = baseX + dx * (attempts + 1);
                    portalY = baseY + dy * (attempts + 1);
                }

                // If still occupied after max attempts, try perpendicular offset
                if (isOccupied(portalX, portalY, baseZ)) {
                    // Add a small perpendicular offset
                    const perpX = dy !== 0 ? 0.3 : 0;
                    const perpY = dx !== 0 ? 0.3 : 0;
                    portalX += perpX;
                    portalY += perpY;
                }

                elements.push({
                    data: {
                        id: portalId,
                        label: destAreaName,
                        isPortal: true,
                        destAreaId: destAreaId,
                        destVnum: destVnum,
                        fromVnum: parseInt(vnum),
                        direction: parseInt(dir),
                        x: portalX,
                        y: portalY,
                        z: baseZ,
                    }
                });

                // Add edge to portal
                elements.push({
                    data: {
                        id: `edge-to-${portalId}`,
                        source: `room-${vnum}`,
                        target: portalId,
                        direction: parseInt(dir),
                        oneWay: exit.one_way,
                        isPortalEdge: true,
                    }
                });

                addedPortals.add(portalId);
            }
        }
    }

    // Destroy existing instance
    if (cy) {
        cy.destroy();
    }

    cy = cytoscape({
        container: document.getElementById('cy'),
        elements: elements,
        style: [
            {
                selector: 'node',
                style: {
                    'label': function(ele) {
                        return currentTextSize > 0 ? ele.data('label') : '';
                    },
                    'background-color': function(ele) {
                        if (ele.data('deadEnd')) return '#ff4444';
                        return SECTOR_COLORS[ele.data('sector')] || '#808080';
                    },
                    'color': '#fff',
                    'text-valign': 'bottom',
                    'text-halign': 'center',
                    'font-size': function() { return currentTextSize + 'px'; },
                    'text-wrap': 'wrap',
                    'text-max-width': function() { return currentWrapWidth + 'px'; },
                    'text-margin-y': 5,
                    'width': 20,
                    'height': 20,
                    'border-width': 2,
                    'border-color': '#333',
                }
            },
            {
                selector: 'node:selected',
                style: {
                    'border-color': '#fff',
                    'border-width': 3,
                }
            },
            {
                selector: 'node[?deadEnd]',
                style: {
                    'shape': 'diamond',
                }
            },
            {
                selector: 'node[?isPortal]',
                style: {
                    'background-color': '#9b59b6',
                    'shape': 'pentagon',
                    'border-color': '#8e44ad',
                    'border-width': 2,
                    'width': 18,
                    'height': 18,
                }
            },
            {
                selector: 'edge[?isPortalEdge]',
                style: {
                    'line-style': 'dashed',
                    'line-color': '#9b59b6',
                    'target-arrow-shape': 'triangle',
                    'target-arrow-color': '#9b59b6',
                    'arrow-scale': 0.8,
                }
            },
            {
                selector: 'edge',
                style: {
                    'width': 2,
                    'line-color': '#666',
                    'curve-style': 'bezier',
                }
            },
            {
                selector: 'edge[?oneWay]',
                style: {
                    'line-style': 'dashed',
                    'line-color': '#f0ad4e',
                    'target-arrow-shape': 'triangle',
                    'target-arrow-color': '#f0ad4e',
                    'arrow-scale': 0.8,
                }
            },
            {
                selector: 'edge[?warped]',
                style: {
                    'line-style': 'dotted',
                    'line-color': '#d9534f',
                    'curve-style': 'unbundled-bezier',
                    'control-point-distances': [50],
                    'control-point-weights': [0.5],
                    'opacity': 0.7,
                    'z-index': 0,  // Draw behind other edges
                }
            },
            {
                selector: 'edge.hidden-warped',
                style: {
                    'display': 'none',
                }
            },
            {
                selector: 'edge[?door]',
                style: {
                    'line-color': '#888',
                    'width': 3,
                }
            },
            {
                selector: 'edge[?bidirectional]',
                style: {
                    'target-arrow-shape': 'none',
                }
            },
            {
                selector: '.hidden',
                style: {
                    'display': 'none',
                }
            },
            {
                selector: '.dimmed',
                style: {
                    'opacity': 0.15,
                    'label': '',  // Hide label when dimmed
                }
            },
        ],
        layout: {
            name: 'preset',
            positions: function(node) {
                // Use coordinates from data, scale up for visibility
                const scale = 50;
                return {
                    x: (node.data('x') || 0) * scale,
                    y: -(node.data('y') || 0) * scale, // Flip Y so north is up
                };
            },
        },
        minZoom: 0.1,
        maxZoom: 5,
        wheelSensitivity: 0.3,
    });

    // Fit to view
    cy.fit(50);

    // Hide warped edges by default
    updateWarpedEdgeVisibility();

    // Node click handler
    cy.on('tap', 'node', function(evt) {
        const node = evt.target;
        showRoomInfo(node.data());
    });

    // Background click to clear selection
    cy.on('tap', function(evt) {
        if (evt.target === cy) {
            document.getElementById('room-info').innerHTML =
                '<p>Click a room node to see details.</p>';
        }
    });
}

function showRoomInfo(data) {
    // Check if this is a portal node
    if (data.isPortal) {
        showPortalInfo(data);
        return;
    }

    let exitsHtml = '<ul class="exit-list">';
    for (const [dir, exit] of Object.entries(data.exits)) {
        let classes = [];
        if (exit.one_way) classes.push('one-way');
        if (exit.warped) classes.push('warped');
        const classStr = classes.length ? `class="${classes.join(' ')}"` : '';

        let suffix = '';
        if (exit.one_way) suffix += ' (one-way)';
        if (exit.warped) suffix += ' (warped)';
        if (exit.door) suffix += ' [door]';

        exitsHtml += `<li ${classStr}>${DIR_NAMES[dir]}: ${exit.to}${suffix}</li>`;
    }
    exitsHtml += '</ul>';

    const html = `
        <p><strong>${data.label}</strong></p>

        <span class="info-label">VNUM</span>
        <span class="info-value">${data.vnum}</span>

        <span class="info-label">Sector</span>
        <span class="info-value">${data.sectorName}</span>

        <span class="info-label">Coordinates</span>
        <span class="info-value">(${data.x}, ${data.y}, ${data.z})</span>

        ${data.deadEnd ? '<p style="color: #ff4444; margin-top: 0.5rem;">Dead-end room!</p>' : ''}

        <span class="info-label">Exits</span>
        ${Object.keys(data.exits).length > 0 ? exitsHtml : '<span class="info-value">None</span>'}
    `;
    document.getElementById('room-info').innerHTML = html;
}

function showPortalInfo(data) {
    const dirName = DIR_NAMES[data.direction] || 'Unknown';
    const jumpLink = data.destAreaId
        ? `<a href="area.html?area=${data.destAreaId}" class="view-area-btn">Jump to ${data.label}</a>`
        : '<span class="info-value">Area not found</span>';

    const html = `
        <p><strong style="color: #9b59b6;">⬠ Portal to Another Area</strong></p>

        <span class="info-label">Destination Area</span>
        <span class="info-value">${data.label}</span>

        <span class="info-label">Destination VNUM</span>
        <span class="info-value">${data.destVnum}</span>

        <span class="info-label">Direction from Room</span>
        <span class="info-value">${dirName} from room ${data.fromVnum}</span>

        ${jumpLink}
    `;
    document.getElementById('room-info').innerHTML = html;
}

function updateAreaStats() {
    const rooms = Object.values(currentArea.rooms);
    const oneWayCount = rooms.reduce((sum, r) =>
        sum + Object.values(r.exits).filter(e => e.one_way).length, 0);
    const deadEndCount = rooms.filter(r => r.dead_end).length;
    const warpedCount = rooms.reduce((sum, r) =>
        sum + Object.values(r.exits).filter(e => e.warped).length, 0);

    // Count cross-area connections (portals)
    let portalCount = 0;
    for (const room of rooms) {
        for (const exit of Object.values(room.exits)) {
            if (!currentArea.rooms[exit.to]) {
                portalCount++;
            }
        }
    }

    const html = `
        <div class="stat-item">
            <span class="label">Total Rooms</span>
            <span class="value">${currentArea.room_count}</span>
        </div>
        <div class="stat-item">
            <span class="label">VNUM Range</span>
            <span class="value">${currentArea.vnum_range[0]} - ${currentArea.vnum_range[1]}</span>
        </div>
        <div class="stat-item">
            <span class="label">One-way Exits</span>
            <span class="value">${oneWayCount}</span>
        </div>
        <div class="stat-item">
            <span class="label">Dead-end Rooms</span>
            <span class="value">${deadEndCount}</span>
        </div>
        <div class="stat-item">
            <span class="label">Non-Euclidean</span>
            <span class="value">${warpedCount}</span>
        </div>
        <div class="stat-item">
            <span class="label">Area Portals</span>
            <span class="value">${portalCount}</span>
        </div>
    `;
    document.getElementById('area-stats').innerHTML = html;
}

function filterByZLevel(z) {
    if (!cy) return;

    if (z === null) {
        // Show all
        cy.nodes().removeClass('hidden dimmed');
        cy.edges().removeClass('hidden');
        document.getElementById('z-level').textContent = 'All';
    } else {
        cy.nodes().forEach(node => {
            if (node.data('z') === z) {
                node.removeClass('hidden dimmed');
            } else {
                node.addClass('dimmed');
            }
        });
        document.getElementById('z-level').textContent = z;
    }
    currentZLevel = z;
}

function updateWarpedEdgeVisibility() {
    if (!cy) return;

    if (showWarpedEdges) {
        cy.edges('[?warped]').removeClass('hidden-warped');
    } else {
        cy.edges('[?warped]').addClass('hidden-warped');
    }
}

function setupEventListeners() {
    // Area selector
    document.getElementById('area-select').addEventListener('change', (e) => {
        if (e.target.value) {
            loadArea(e.target.value);
        }
    });

    // Search
    const searchInput = document.getElementById('search-input');
    const searchBtn = document.getElementById('search-btn');

    function doSearch() {
        if (!cy) return;
        const query = searchInput.value.toLowerCase().trim();
        if (!query) {
            cy.nodes().removeClass('dimmed');
            return;
        }

        cy.nodes().forEach(node => {
            const matches = node.data('label').toLowerCase().includes(query) ||
                           node.data('vnum').toString().includes(query);
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
        if (!searchInput.value && cy) {
            cy.nodes().removeClass('dimmed');
        }
    });

    // Z-level controls
    document.getElementById('z-up').addEventListener('click', () => {
        if (currentZLevel === null) {
            filterByZLevel(0);
        } else {
            filterByZLevel(currentZLevel + 1);
        }
    });

    document.getElementById('z-down').addEventListener('click', () => {
        if (currentZLevel === null) {
            filterByZLevel(0);
        } else {
            filterByZLevel(currentZLevel - 1);
        }
    });

    document.getElementById('z-all').addEventListener('click', () => {
        filterByZLevel(null);
    });

    // Text size slider
    const textSizeSlider = document.getElementById('text-size');
    const textSizeValue = document.getElementById('text-size-value');

    textSizeSlider.addEventListener('input', (e) => {
        currentTextSize = parseInt(e.target.value);
        textSizeValue.textContent = currentTextSize > 0 ? currentTextSize + 'px' : 'Off';
        if (cy) {
            cy.style().update();
        }
    });

    // Wrap width slider
    const wrapSlider = document.getElementById('text-wrap');
    const wrapValue = document.getElementById('text-wrap-value');

    wrapSlider.addEventListener('input', (e) => {
        currentWrapWidth = parseInt(e.target.value);
        wrapValue.textContent = currentWrapWidth + 'px';
        if (cy) {
            cy.style().update();
        }
    });

    // Warped edges toggle
    const warpedToggle = document.getElementById('show-warped');
    if (warpedToggle) {
        warpedToggle.checked = showWarpedEdges;
        warpedToggle.addEventListener('change', (e) => {
            showWarpedEdges = e.target.checked;
            updateWarpedEdgeVisibility();
        });
    }
}

// Start the app
document.addEventListener('DOMContentLoaded', init);
